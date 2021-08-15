//
//  ap242P21DecodeManager.swift
//  MultiP21reads
//
//  Created by Yoshida on 2021/08/14.
//  Copyright © 2021 Tsutomu Yoshida, Minokamo, Japan. All rights reserved.
//

import Foundation
import SwiftSDAIcore
import SwiftSDAIap242

public class AP242P21DecodeManager: SDAI.Object {
	
	public enum P21Status: Equatable {
		case notLoadedYet
		case loadPending
		case loadedSuccessfully
		case loadedWithWarnings(String)
		case inURLError(String)
		case inDecoderError(P21Decode.Decoder.Error?)
	}
	
	public class P21Entry: SDAI.Object {
		var status: P21Status = .notLoadedYet
		var p21URL: URL
		var fileName: String { p21URL.lastPathComponent }
		var prefix: String
		var entryName: String { prefix + fileName }
		var exchangeStructure: P21Decode.ExchangeStructure?
		var sdaiModels: [SDAIPopulationSchema.SdaiModel]?
		
		init(url: URL, prefix: String) {
			self.p21URL = url
			self.prefix = prefix
			super.init()
		}
	}
	
	public struct ExternalReferenceIdentity: Equatable {
		var shapeRepresentationID: ap242.tIDENTIFIER?
		var shapeRepresentationName: ap242.tLABEL
		var productID: ap242.tIDENTIFIER
		var productName: ap242.tLABEL
	}
	
	public class ExternalReferenceEntry: P21Entry {
		var upStream: P21Entry
		var documentFormat: ap242.tTEXT
		var expectedIdentity: ExternalReferenceIdentity
		var loadedIdentity: ExternalReferenceIdentity?
		
		init(upStream: P21Entry, 
				 url: URL, 
				 prefix: String,
				 format: ap242.tTEXT,
				 expected: ExternalReferenceIdentity) {
			self.upStream = upStream
			self.documentFormat = format
			self.expectedIdentity = expected
			super.init(url: url, prefix: prefix)
		}
	}
	
	open class ActivityMonitor: P21Decode.ActivityMonitor {
		open func startedLoading(p21entry: P21Entry) {}
		open func completedLoading(p21entry: P21Entry) {}
		open func identified(externalReferences: [ExternalReferenceEntry], originatedFrom p21entry: P21Entry) {}
	}
	
	private var decoder: P21Decode.Decoder
	private var monitor: ActivityMonitor?
	public var p21entries: [String:P21Entry] = [:]
	
	public var models: AnySequence<SDAIPopulationSchema.SdaiModel> {
		AnySequence(p21entries.values.lazy.compactMap({$0.sdaiModels}).joined())
	}
	
	public init?(repository: SDAISessionSchema.SdaiRepository,
							schemaList: P21Decode.SchemaList,
							masterFile: URL,
							monitor: ActivityMonitor?,
							foreginReferenceResolver: P21Decode.ForeignReferenceResolver = P21Decode.ForeignReferenceResolver() 
	) {
		guard let decoder = P21Decode.Decoder(
						output: repository, 
						schemaList: schemaList,
						monitor: monitor,
						foreginReferenceResolver: foreginReferenceResolver) 
		else { return nil }
		self.decoder = decoder
		self.monitor = monitor
		
		let master = P21Entry(url: masterFile, prefix: "(master)")
		self.p21entries[master.fileName] = master
		
		super.init()
	}
	
	public func decode() {
		var needToLoad = true
		var round = 0
		while needToLoad {
			needToLoad = false
			round += 1
			
			for entry in p21entries.values {
				if entry.status != .notLoadedYet { continue }
				if !load(p21entry:entry) { continue }
				
				let extrefs = identifyExternalReferences(p21entry:entry, round:round)
				monitor?.identified(externalReferences:extrefs, originatedFrom:entry)
				for extref in extrefs {
					p21entries[extref.fileName] = extref
					needToLoad = true
				}
			}
		}
	}
	
	private func load(p21entry: P21Entry) -> Bool {
		monitor?.startedLoading(p21entry: p21entry)
		defer {
			monitor?.completedLoading(p21entry: p21entry)
		}
		do {
			let source = try String(contentsOf: p21entry.p21URL)
			let stream = source.makeIterator()
			guard let models = decoder.decode(input: stream), 
						let exchange = decoder.exchangeStructrure 
			else {
				p21entry.status = .inDecoderError(decoder.error)
				return false
			}
			p21entry.sdaiModels = models
			p21entry.exchangeStructure = exchange
			p21entry.status = .loadedSuccessfully
			
			if let extref = p21entry as? ExternalReferenceEntry {
				confirmIdentity(extRef: extref)
			}
		} 
		catch {
			p21entry.status = .inURLError("failed to obtain contents from url:\(p21entry.p21URL)")
			return false
		}
		return true
	}
	
	private func identifyExternalReferences(p21entry: P21Entry, round:Int) -> [ExternalReferenceEntry] {
		var result: [ExternalReferenceEntry] = []
		guard let models = p21entry.sdaiModels else { return result }
		
		let repository = decoder.repository
		let schemaInstance = repository.createSchemaInstance(name: p21entry.fileName+".TEMP", schema: ap242.schemaDefinition)
		for model in models {
			guard model.underlyingSchema == schemaInstance.nativeSchema else { continue }
			schemaInstance.add(model: model)
		}
		schemaInstance.mode = .readOnly
		
		// document file
		for documentFile in schemaInstance.entityExtent(type: ap242.eDOCUMENT_FILE.self) {
			// file name
			var filename = ""
			if let usedin = SDAI.USEDIN(T: documentFile, ROLE: \ap242.eAPPLIED_EXTERNAL_IDENTIFICATION_ASSIGNMENT.ITEMS), 
				 !usedin.isEmpty, let AEIA = usedin[1] {
				filename = AEIA.ASSIGNED_ID.asSwiftType
				if filename.isEmpty {
					filename = AEIA.SOURCE.SOURCE_ID.asSwiftString
				}
			}
			else {
				filename = documentFile.ID.asSwiftType
			}
			if filename.isEmpty { continue }
			
			//shape representation
			guard let extdefProp = SDAI.USEDIN(T: documentFile, 
																			ROLE: \ap242.ePROPERTY_DEFINITION.DEFINITION)?
							.asSwiftType
							.filter({$0.NAME.asSwiftType == "external definition"}),
						extdefProp.count == 1
			else { continue }
			
			guard let extdefPropRep = SDAI.USEDIN(T: extdefProp[0], 
																				 ROLE: \ap242.ePROPERTY_DEFINITION_REPRESENTATION.DEFINITION)?
							.asSwiftType,
						extdefPropRep.count == 1
			else { continue }
			
			let shapeRep = extdefPropRep[0].USED_REPRESENTATION
			
			//product
			guard let shapeDefRep = SDAI.USEDIN(T: shapeRep, 
																					ROLE: \ap242.eSHAPE_DEFINITION_REPRESENTATION.USED_REPRESENTATION)?
							.asSwiftType,
						shapeDefRep.count == 1
			else { continue }
			
			guard let product = shapeDefRep[0].DEFINITION.DEFINITION.FORMATION?.OF_PRODUCT
			else { continue }
			
			//descritive representation item
			guard let docpropProp = SDAI.USEDIN(T: documentFile, 
																			ROLE: \ap242.ePROPERTY_DEFINITION.DEFINITION)?
							.asSwiftType
							.filter({$0.NAME.asSwiftType == "document property"}),
						docpropProp.count == 1
			else { continue }
			
			guard let docpropPropRep = SDAI.USEDIN(T: docpropProp[0], 
																				 ROLE: \ap242.ePROPERTY_DEFINITION_REPRESENTATION.DEFINITION)?
							.asSwiftType,
						docpropPropRep.count == 1
			else { continue }
		
			let descRepItem = docpropPropRep[0].USED_REPRESENTATION.ITEMS
				.asSwiftType.compactMap({$0.GROUP_REF(ap242.eDESCRIPTIVE_REPRESENTATION_ITEM.self)})
				.filter({$0.NAME.asSwiftType == "data format"})
			guard descRepItem.count == 1 
			else { continue }
			
			//external reference
			let url = URL(fileURLWithPath: p21entry.p21URL.deletingLastPathComponent().path + "/" + filename )
			let format = descRepItem[0].DESCRIPTION
			let ident = ExternalReferenceIdentity(shapeRepresentationID: shapeRep.ID, 
																						shapeRepresentationName: shapeRep.NAME, 
																						productID: product.ID, 
																						productName: product.NAME)
			let extref = ExternalReferenceEntry(upStream: p21entry, 
																					url: url, 
																					prefix: "(sub.\(round))", 
																					format: format, 
																					expected: ident)
			result.append(extref)
		}

		repository.deleteSchemaInstance(instance: schemaInstance)
		return result
	}
	
	private func confirmIdentity(extRef: ExternalReferenceEntry) {
		guard let models = extRef.sdaiModels else { return }
		
		let repository = decoder.repository
		let schemaInstance = repository.createSchemaInstance(name: extRef.fileName+".TEMP", schema: ap242.schemaDefinition)
		defer {
			repository.deleteSchemaInstance(instance: schemaInstance)
		}
		for model in models {
			guard model.underlyingSchema == schemaInstance.nativeSchema else { continue }
			schemaInstance.add(model: model)
		}
		schemaInstance.mode = .readOnly

		// product
		var productIDs: [SDAI.STRING] = []
		for product in schemaInstance.entityExtent(type: ap242.ePRODUCT.self) {
			productIDs.append(SDAI.STRING(product.ID))
			if product.ID != extRef.expectedIdentity.productID { continue }
			
			// shape definition representation
			let shapeDefRep = Array( schemaInstance
				.entityExtent(type: ap242.eSHAPE_DEFINITION_REPRESENTATION.self)
				.lazy
				.filter { $0.DEFINITION.DEFINITION.FORMATION?.OF_PRODUCT == product } )
			guard shapeDefRep.count == 1
			else { 
				extRef.status = .loadedWithWarnings("loaded data does not contain unique SHAPE DEFINITION REPRESENTATION")
				return
			}
			
			// shape representation
			let shapeRep = shapeDefRep[0].USED_REPRESENTATION
			
			// loaded identity
			let ident = ExternalReferenceIdentity(shapeRepresentationID: shapeRep.ID,
																						shapeRepresentationName: shapeRep.NAME, 
																						productID: product.ID, 
																						productName: product.NAME)
			extRef.loadedIdentity = ident
			if extRef.expectedIdentity != ident {
				extRef.status = .loadedWithWarnings("loaded PRODUCT itentity is not consistent with expected")
			}
			return
		}
		extRef.status = .loadedWithWarnings("PRODUCT with expected ID(\(extRef.expectedIdentity.productID.asSwiftType)) is not contained; found PRODUCTs:\(productIDs)")
	}
	
}

