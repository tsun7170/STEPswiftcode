//
//  main.swift
//  swiftP21read
//
//  Created by Yoshida on 2020/09/09.
//  Copyright © 2020 Tsutomu Yoshida, Minokamo, Japan. All rights reserved.
//

import Foundation
import SwiftSDAIcore
import SwiftSDAIap242

print("swiftP21read")

let testDataFolder = ProcessInfo.processInfo.environment["TEST_DATA_FOLDER"]!
//let url = URL(fileURLWithPath: testDataFolder + "NIST_CTC_STEP_PMI/nist_ctc_02_asme1_ap242-e2.stp")
//let url = URL(fileURLWithPath: testDataFolder + "CAx STEP FILE LIBRARY/sg1-c5-214.stp")
let url = URL(fileURLWithPath: testDataFolder + "CAx STEP FILE LIBRARY/s1-c5-214/s1-c5-214.stp")
//let url = URL(fileURLWithPath: testDataFolder + "CAx STEP FILE LIBRARY/s1-c5-214/TAIL_TURBINE.stp")

let stepsource = try String(contentsOf: url) 

var charstream = stepsource.makeIterator()

let repository = SDAISessionSchema.SdaiRepository(name: "examle", description: "example repository")

let schemaList: P21Decode.SchemaList = [
	"AP242_MANAGED_MODEL_BASED_3D_ENGINEERING_MIM_LF { 1 0 10303 442 3 1 4 }": AP242_MANAGED_MODEL_BASED_3D_ENGINEERING_MIM_LF.self,
	"AP242_MANAGED_MODEL_BASED_3D_ENGINEERING_MIM_LF { 1 0 10303 442 1 1 4 }": AP242_MANAGED_MODEL_BASED_3D_ENGINEERING_MIM_LF.self,
	"AP203_CONFIGURATION_CONTROLLED_3D_DESIGN_OF_MECHANICAL_PARTS_AND_ASSEMBLIES_MIM_LF  { 1 0 10303 403 3 1 4}": AP242_MANAGED_MODEL_BASED_3D_ENGINEERING_MIM_LF.self,
	"AP203_CONFIGURATION_CONTROLLED_3D_DESIGN_OF_MECHANICAL_PARTS_AND_ASSEMBLIES_MIM_LF { 1 0 10303 403 2 1 2}": AP242_MANAGED_MODEL_BASED_3D_ENGINEERING_MIM_LF.self,
	"CONFIG_CONTROL_DESIGN": AP242_MANAGED_MODEL_BASED_3D_ENGINEERING_MIM_LF.self,
	"CONFIGURATION_CONTROL_3D_DESIGN_ED2_MIM_LF { 1 0 10303 403 1 1 4}": AP242_MANAGED_MODEL_BASED_3D_ENGINEERING_MIM_LF.self,
	"AUTOMOTIVE_DESIGN { 1 0 10303 214 1 1 1 1 }": AP242_MANAGED_MODEL_BASED_3D_ENGINEERING_MIM_LF.self,
]

//MARK: decode p21
let p21monitor = MyActivityMonitor()

guard let decoder = P21Decode.Decoder(output: repository, schemaList: schemaList, monitor: p21monitor)
else {
	print("decoder initialization error")
	exit(1)
}

 guard let createdModels = decoder.decode(input: charstream) else {
	print("decoder error: \(String(describing: decoder.error))")
	exit(2)
}
let exchange = decoder.exchangeStructrure!
guard let schema = exchange.shcemaRegistory.values.first else { exit(3) }
let schemaInstance = repository.createSchemaInstance(name: "example", schema: schema.schemaDefinition)
for model in createdModels {
	schemaInstance.add(model:model)
}
schemaInstance.mode = .readOnly


//MARK:- print notable entities
do{
	//MARK: product
	let entityType = ap242.ePRODUCT.self
	let instances = schemaInstance.entityExtent(type: entityType)
	print("\(entityType):")
	for (i,instance) in instances.enumerated() {
		print(" [\(i)]\t\(instance) .ID=\(instance.ID) .NAME=\(instance.NAME)")
		
		//MARK: product_definition_formation
		if let usedin = SDAI.USEDIN(T: instance, ROLE: \ap242.ePRODUCT_DEFINITION_FORMATION.OF_PRODUCT) {
			for (j,instance) in usedin.enumerated() {
				print(" [\(i).\(j)]\t\(instance) .ID=\(instance.ID)")
				
				//MARK: product_definition
				if let usedin = SDAI.USEDIN(T: instance, ROLE: \ap242.ePRODUCT_DEFINITION.FORMATION) {
					for (k,instance) in usedin.enumerated() {
						print(" [\(i).\(j).\(k)]\t\(instance) .ID=\(instance.ID)")
						
						//MARK: product_definition_shape
						if let usedin = SDAI.USEDIN(T: instance, ROLE: \ap242.ePRODUCT_DEFINITION_SHAPE.DEFINITION) {
							for (l,instance) in usedin.enumerated() {
								print(" [\(i).\(j).\(k).\(l)]\t\(instance) .NAME=\(instance.NAME)")
								
								//MARK: shape_definition_representation
								if let usedin = SDAI.USEDIN(T: instance, ROLE: \ap242.eSHAPE_DEFINITION_REPRESENTATION.DEFINITION) {
									for (m,instance) in usedin.enumerated() {
										print(" [\(i).\(j).\(k).\(l).\(m)]\t\(instance) .USED_REPRESENTATION=\(instance.USED_REPRESENTATION)") // shape_representation
									}
								}
								if !usedin.isEmpty {print("--")}
								//MARK: shape_aspect
								if let usedin = SDAI.USEDIN(T: instance, ROLE: \ap242.eSHAPE_ASPECT.OF_SHAPE) {
									for (m,instance) in usedin.enumerated() {
										print(" [\(i).\(j).\(k).\(l).\(m)]\t\(instance)")
										
										//MARK: geometric_item_specific_usage
										if let usedin = SDAI.USEDIN(T: instance, ROLE: \ap242.eGEOMETRIC_ITEM_SPECIFIC_USAGE.DEFINITION) {
											for (n,instance) in usedin.enumerated() {
												print(" [\(i).\(j).\(k).\(l).\(m).\(n)]\t\(instance) .USED_REPRESENTATION=\(instance.USED_REPRESENTATION)") // constructive_geometry_representation
											}
										}
									}
								}
							}
							if !usedin.isEmpty {print("--")}
						}
						//MARK: applied_document_reference
						if let usedin = SDAI.USEDIN(T: instance, ROLE: \ap242.eAPPLIED_DOCUMENT_REFERENCE.ITEMS) {
							for (l,instance) in usedin.enumerated() {
								print(" [\(i).\(j).\(k).\(l)]\t\(instance) .SOURCE=\(instance.SOURCE)")
							}
//							print("--")
						}
					}
				}
			}
		}		
	}
	print("")
}
	
do{
	//MARK: - shape_representation
	let entityType = ap242.eSHAPE_REPRESENTATION.self
	let instances = schemaInstance.entityExtent(type: entityType)
	print("\(entityType):")
	for (i,instance) in instances.enumerated() {
		print(" [\(i)]\t\(instance) .ID=\(instance.ID ?? "?") .NAME=\(instance.NAME)")
		for (i1,item) in instance.ITEMS.enumerated() {
			print(" .ITEM[\(i1)]\t\(item.complexEntity)")
		}
		
		//MARK: shape_representation_relationship
		if let usedin = SDAI.USEDIN(T: instance, ROLE: \ap242.eSHAPE_REPRESENTATION_RELATIONSHIP.REP_1) {
			for (j,instance) in usedin.enumerated() {
				print(" [\(i).\(j)]\t\(instance) \n\t .REP_2=\(instance.REP_2)")
				continue
			}
			if !usedin.isEmpty {print("--")}
		}
		
		//MARK: property_definition_representation
		if let usedin = SDAI.USEDIN(T: instance, ROLE: \ap242.ePROPERTY_DEFINITION_REPRESENTATION.USED_REPRESENTATION) {
			for (j,instance) in usedin.enumerated() {
				let prop_def = instance.DEFINITION
				let doc_file = prop_def.DEFINITION
				print(" [\(i).\(j)]\t\(instance) .DEFINITION=\(prop_def) .DEFINITION.NAME=\(prop_def.NAME ?? "?")\n\t .DEFINITION.DEFINITION=\(String(describing: doc_file)) .DEFINITION.DEFINITION.ID=\(doc_file?.GROUP_REF(ap242.eDOCUMENT_FILE.self)?.ID ?? "?")")
				continue
			}
			if !usedin.isEmpty {print("--")}
		}
		//MARK: constructive_geometry_representation_relationship
		if let usedin = SDAI.USEDIN(T: instance, ROLE: \ap242.eCONSTRUCTIVE_GEOMETRY_REPRESENTATION_RELATIONSHIP.REP_1) {
			for (j,instance) in usedin.enumerated() {
				print(" [\(i).\(j)]\t\(instance) \n\t .REP_2=\(instance.REP_2)")
				continue
			}
		}
	}
	print("")
}
do{
	//MARK: - constructive_geometry_representation
	let entityType = ap242.eCONSTRUCTIVE_GEOMETRY_REPRESENTATION.self
	let instances = schemaInstance.entityExtent(type: entityType)
	print("\(entityType):")
	for (i,instance) in instances.enumerated() {
		print(" [\(i)]\t\(instance)")
		for (i1,item) in instance.ITEMS.enumerated() {
			print(" .ITEM[\(i1)]\t\(item.complexEntity)")
		}
	}
	print("")
}
do{
	//MARK: applied_document_reference
	let entityType = ap242.eAPPLIED_DOCUMENT_REFERENCE.self
	let instances = schemaInstance.entityExtent(type: entityType)
	print("\(entityType):")
	for (i,instance) in instances.enumerated() {
		print(" [\(i)]\t\(instance) .ASSIGNED_DOCUMENT=\(instance.ASSIGNED_DOCUMENT) .ITEMS=\(instance.ITEMS)")
		
		//MARK: document_product_equivalence
		if let usedin = SDAI.USEDIN(T: instance.ASSIGNED_DOCUMENT, ROLE: \ap242.eDOCUMENT_PRODUCT_EQUIVALENCE.RELATING_DOCUMENT) {
			for (j,instance) in usedin.enumerated() {
				print(" [\(i).\(j)]\t\(instance) .RELATED_PRODUCT=\(instance.RELATED_PRODUCT)")
				
				//MARK: product_definition_withassociated_documents
				if let usedin = SDAI.USEDIN(T: instance.RELATED_PRODUCT, ROLE: \ap242.ePRODUCT_DEFINITION_WITH_ASSOCIATED_DOCUMENTS.FORMATION) {
					for (k,instance) in usedin.enumerated() {
						print(" [\(i).\(j).\(k)]\t\(instance) .DOCUMENTATION_IDS=\(instance.DOCUMENTATION_IDS)") // document_file
					}
				}
			}
		}
	}
	print("")
}
do{
	//MARK: - applied_external_identification_assignment
	let entityType = ap242.eAPPLIED_EXTERNAL_IDENTIFICATION_ASSIGNMENT.self
	let instances = schemaInstance.entityExtent(type: entityType)
	print("\(entityType):")
	for (i,instance) in instances.enumerated() {
		print(" [\(i)]\t\(instance) .ASSIGNED_ID=\(instance.ASSIGNED_ID) .SOURCE=\(instance.SOURCE) .ITEMS:\(instance.ITEMS)")
	}
	print("")
}
do{
	//MARK: - external_source
	let entityType = ap242.eEXTERNAL_SOURCE.self
	let instances = schemaInstance.entityExtent(type: entityType)
	print("\(entityType):")
	for (i,instance) in instances.enumerated() {
		print(" [\(i)]\t\(instance) .SOURCE_ID=\(instance.SOURCE_ID)")
	}
	print("")
}
do{
	//MARK: - document_file
	let entityType = ap242.eDOCUMENT_FILE.self
	let instances = schemaInstance.entityExtent(type: entityType)
	print("\(entityType):")
	for (i,instance) in instances.enumerated() {
		print(" [\(i)]\t\(instance) .ID=\(instance.ID)")
		
		//MARK: property_definition
		if let usedin = SDAI.USEDIN(T: instance, ROLE: \ap242.ePROPERTY_DEFINITION.DEFINITION) {
			for (j,instance) in usedin.enumerated() {
				print(" [\(i).\(j)]\t\(instance) .NAME=\(instance.NAME)")
				
				//MARK: property_definition_representation
				if let usedin = SDAI.USEDIN(T: instance, ROLE: \ap242.ePROPERTY_DEFINITION_REPRESENTATION.DEFINITION) {
					for (k,instance) in usedin.enumerated() {
						let representation = instance.USED_REPRESENTATION
						print(" [\(i).\(j).\(k)]\t\(instance) .USED_REPRESENTATION=\(representation)")
						for (l,item) in representation.ITEMS.enumerated() {
							if let descritive_rep_item = item.GROUP_REF(ap242.eDESCRIPTIVE_REPRESENTATION_ITEM.self) {
								print(" .ITEMS[\(l)]\t\(item) .NAME=\(item.NAME) .DESCRIPTION=\(descritive_rep_item.DESCRIPTION)")
							}
						}
					}
				}
			}
		}

	}
	print("")
}

//MARK: - find top level entities
var printTopLevels = false
if printTopLevels {
	let toplevel = exchange.topLevelEntities
	let numToplevel = toplevel.count
	let numTotal = exchange.entityInstanceRegistory.count
	print("decoded \(numTotal) entities")
	print("top level entities (\(numToplevel)): \(toplevel)\n")
	
	for (name,instances) in toplevel {
		if let some = instances.first, let instance = exchange.entityInstanceRegistory[some], let complex = instance.resolved {
			print("\(name) #\(some): \(complex)")
			print("")
		}
	}
}

//MARK: validation

let validationMonitor = MyValidationMonitor()

var doIndividualWhereValidation = false
if doIndividualWhereValidation {
	let entityType = ap242.eANNOTATION_OCCURRENCE.self	// WHERE_wr1
//	let entityType = ap242.eANNOTATION_PLACEHOLDER_OCCURRENCE.self	// WHERE_wr1
//	let entityType = ap242.eDRAUGHTING_MODEL.self	// WHERE_wr2
//	let entityType = ap242.eFOUNDED_ITEM.self	// WHERE_wr2
//	let entityType = ap242.eMECHANICAL_DESIGN_GEOMETRIC_PRESENTATION_REPRESENTATION.self	// WHERE_wr8
//	let entityType = ap242.ePLACED_DATUM_TARGET_FEATURE.self	// WHERE_wr3
//	let entityType = ap242.eREPRESENTATION_ITEM.self	// WHERE_wr1
//	let entityType = ap242.eTESSELLATED_ITEM.self	// WHERE_wr1
//	let entityType = ap242.eTESSELLATED_SHAPE_REPRESENTATION.self	// WHERE_wr2


	let instances = schemaInstance.entityExtent(type: entityType)
	for (i,entity) in instances.enumerated() {
		let result = type(of: entity.partialEntity).WHERE_wr1(SELF: entity)
		print("[\(i)] \(entity): \(result)")
		continue
	}
}

var doEntityValidation = false
if doEntityValidation {
	let entityType = ap242.eANNOTATION_PLACEHOLDER_OCCURRENCE.self
	let instances = schemaInstance.entityExtent(type: entityType)
	for (i,entity) in instances.enumerated() {
		let result = entityType.validateWhereRules(instance: entity, prefix: "\(entity): ")
		print("[\(i)] \(result)")
		continue
	}
}

var doGlobalRuleValidation = false
if doGlobalRuleValidation {
	let globalResult = schemaInstance.validateGlobalRules(monitor:validationMonitor)
	print("\n glovalRuleValidationRecord(\(globalResult.count)):\n\(globalResult)"  )
}

var doUniqunessRuleValidation = false
if doUniqunessRuleValidation {
	let uniquenessResult = schemaInstance.validateUniquenessRules(monitor:validationMonitor)
	print("\n uniquenessRuleValidationRecord(\(uniquenessResult.count)):\n\(uniquenessResult)")
}	

var doWhereRuleValidation = false
if doWhereRuleValidation {
	let whereResult = schemaInstance.validateWhereRules(monitor:validationMonitor)
	print("\n whereRuleValidationRecord:\n\(whereResult)" )
}	

var doAllValidaton = false
if doAllValidaton {
	let validationPassed = schemaInstance.validateAllConstraints(monitor: MyValidationMonitor())
	print("validationPassed:", validationPassed)
	print("glovalRuleValidationRecord: \(String(describing: schemaInstance.globalRuleValidationRecord))"  )
	print("uniquenessRuleValidationRecord: \(String(describing: schemaInstance.uniquenessRuleValidationRecord))")
	print("whereRuleValidationRecord: \(String(describing: schemaInstance.whereRuleValidationRecord))" )
}

//MARK: entity look up
var name = 1
while name != 0 {
	if let instance = exchange.entityInstanceRegistory[name], let complex = instance.resolved {
		print("#\(name): source = \(instance.source)\n complex = \(complex)\n")
		name = 0
		continue
	}
	else {
		name = 0
		continue
	}
}


print("normal end of execution")


