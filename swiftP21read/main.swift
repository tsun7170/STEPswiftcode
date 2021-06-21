//
//  main.swift
//  swiftP21read
//
//  Created by Yoshida on 2020/09/09.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

import Foundation
import SwiftSDAIcore
import SwiftSDAIap242

print("swiftP21read")

let url = URL(fileURLWithPath: "./TestCases/NIST_CTC_STEP_PMI/nist_ctc_02_asme1_ap242-e2.stp")

let stepsource = try String(contentsOf: url) 

var charstream = stepsource.makeIterator()

//var p21stream = P21Decode.P21CharacterStream(charStream: charstream)
//
//while p21stream.lineNumber < 10 {
//	print(p21stream.next() ?? "<nil>", terminator:"")
//}

//let parser = P21Decode.ExchangeStructureParser(charStream: charstream)
//
//let result = parser.parseExchangeStructure()
//
//if result == nil {
//	print("parser error = ",parser.error ?? "unknown error")
//}
//else {
//	print("normal end of execution")
//}

let repository = SDAISessionSchema.SdaiRepository(name: "examle", description: "example repository")

let schemaList: P21Decode.SchemaList = [
	"AP242_MANAGED_MODEL_BASED_3D_ENGINEERING_MIM_LF { 1 0 10303 442 3 1 4 }": AP242_MANAGED_MODEL_BASED_3D_ENGINEERING_MIM_LF.self,
]

//MARK: decode p21
let p21monitor = MyActivityMonitor()

guard let decoder = P21Decode.Decoder(output: repository, schemaList: schemaList, monitor: p21monitor)
else {
	print("decoder initialization error")
	exit(1)
}

let output = decoder.decode(input: charstream)
if output == nil {
	print("decoder error: \(String(describing: decoder.error))")
	exit(2)
}

//MARK: find top level entities
let exchange = decoder.exchangeStructrure!
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

//MARK: validation
guard let schema = exchange.shcemaRegistory.values.first else { exit(3) }
let schemaInstance = SDAIPopulationSchema.SchemaInstance(repository: repository, 
																												 name: "examle", 
																												 schema: schema.schemaDefinition)
for model in repository.contents.models.values {
	schemaInstance.add(model:model)
}
schemaInstance.mode = .readOnly

let validationMonitor = MyValidationMonitor()

let globalResult = schemaInstance.validateGlobalRules(monitor:validationMonitor)
print("\n entity attribute cache hit rate: \(String(describing: SDAI.EntityReference.cacheHitRate))")
print("glovalRuleValidationRecord:\n\(globalResult)"  )

let uniquenessResult = schemaInstance.validateUniquenessRules(monitor:validationMonitor)
print("\n entity attribute cache hit rate: \(String(describing: SDAI.EntityReference.cacheHitRate))")
print("uniquenessRuleValidationRecord:\n\(uniquenessResult)")

let whereResult = schemaInstance.validateWhereRules(monitor:validationMonitor)
print("\n entity attribute cache hit rate: \(String(describing: SDAI.EntityReference.cacheHitRate))")
print("whereRuleValidationRecord:\n\(whereResult)" )


//let validationPassed = schemaInstance.validateAllConstraints(monitor: MyValidationMonitor())
//print("entity attribute cache hit rate: \(String(describing: SDAI.EntityReference.cacheHitRate))")
//print("validationPassed:", validationPassed)
//print("glovalRuleValidationRecord: \(String(describing: schemaInstance.globalRuleValidationRecord))"  )
//print("uniquenessRuleValidationRecord: \(String(describing: schemaInstance.uniquenessRuleValidationRecord))")
//print("whereRuleValidationRecord: \(String(describing: schemaInstance.whereRuleValidationRecord))" )

//MARK: entity look up
var entityType = AP242.eSHAPE_REPRESENTATION.self
let instances = schemaInstance.entityExtent(type: entityType)
print(entityType, instances)

var name = 1
while name != 0 {
	if let instance = exchange.entityInstanceRegistory[name], let complex = instance.resolved {
		print("#\(name): source = \(instance.source)\n\(complex)")
	}
	name = 0
	print("")
}


print("normal end of execution")

//MARK: - activity monitor
class MyActivityMonitor: P21Decode.ActivityMonitor {
	override func tokenStreamDidSet(error p21Error: P21Decode.P21Error) {
		print("error detected on token stream: \(p21Error)")
	}
	
	override func parserDidSet(error p21Error: P21Decode.P21Error) {
		print("error detected on parser: \(p21Error)")
	}
	
	override func exchangeStructureDidSet(error exError: String) {
		print("error detected on exchange structure: \(exError)")
	}
	
	override func decoderDidSet(error decoderError: P21Decode.Decoder.Error) {
		print("error detected on decoder: \(decoderError)")
	}
	
	override func scannerDidDetectNewLine(lineNumber: Int) {
		if lineNumber % 1000 == 0 			{ print("+", terminator: "") }
		else if lineNumber % 100 == 0 	{ print(".", terminator: "") }
	}
	
	var entityCount = 0
	override func decoderResolved(entiyInstanceName: P21Decode.ExchangeStructure.EntityInstanceName) {
		entityCount += 1
		if entityCount % 1000 == 0 			{ print("*", terminator: "") }
		else if entityCount % 100 == 0 	{ print(".", terminator: "") }		
	}
	
	override func startedParsingHeaderSection() {
		print("\n parsing header section: ", terminator: "")
	}
	
	override func startedParsingAnchorSection() {
		print("\n parsing anchor section: ", terminator: "")
	}
	
	override func startedParsingReferenceSection() {
		print("\n parsing reference section: ", terminator: "")
	}
	
	override func startedParsingDataSection() {
		print("\n parsing data section: ", terminator: "")

	}
	
	override func completedParsing() {
		print("\n completed parsing.")
	}
	
	override func startedResolvingEntityInstances() {
		print(" resolving entity instances: ", terminator: "")

	}
	
	override func completedResolving() {
		print("\n completed resolving.")
	}
}

//MARK: - validation monitor
class MyValidationMonitor: SDAIPopulationSchema.ValidationMonitor {
	var globalCount: Int = 0
	var uniquenessCount: Int = 0
	var complexCount: Int = 0
	
	var globalValidated: Int = 0
	var uniquenessValidated: Int = 0
	var complexValidated: Int = 0
	
	override func willValidate(globalRules: AnySequence<SDAIDictionarySchema.GlobalRule>) {
		globalCount = globalRules.reduce(0, { (count,_) in count + 1 })
		print("\n validating \(globalCount) global rules: ", terminator: "")
	}
	
	override func willValidate(uniquenessRules: AnySequence<SDAIDictionarySchema.UniquenessRule>) {
		uniquenessCount =  uniquenessRules.reduce(0, { (count,_) in count + 1 })
		print("\n validating \(uniquenessCount) uniqueness rules: ", terminator: "")
	}
	
	override func willValidateWhereRules(for complexEntites: AnySequence<SDAI.ComplexEntity>) {
		complexCount = complexEntites.reduce(0, { (count,_) in count + 1 })
		print("\n validating where rules for \(complexCount) complex entities: ", terminator: "")
	}
	
	private func progressMarker(completed: Int, total: Int) -> String? {
		if total == 0 { return nil }
		if total <= 100 { return completed % 10 == 0 ? "+" : "." }
		
		if (completed * 10) % total < 10 {
			return "\((completed * 10) / total)"
		}
		if (completed * 50) % total < 50 {
			return "+"
		}
		if ( completed * 100) % total < 100 {
			return "."
		}
		return nil
	}
	
	override func didValidateGlobalRule(for schemaInstance: SDAIPopulationSchema.SchemaInstance, result: SDAI.GlobalRuleValidationResult) {
		globalValidated += 1
		if let marker = progressMarker(completed: globalValidated, total: globalCount) { print(marker, terminator: "") }
		
		if result.result == SDAI.FALSE {
			print("X", terminator: "")
			let _ = schemaInstance.validate(globalRule: result.globalRule)
		}
	}
	
	override func didValidateUniquenessRule(for schemaInstance: SDAIPopulationSchema.SchemaInstance, result: SDAI.UniquenessRuleValidationResult) {
		uniquenessValidated += 1
		if let marker = progressMarker(completed: uniquenessValidated, total: uniquenessCount) { print(marker, terminator: "") }

		if result.result == SDAI.FALSE {
			print("X", terminator: "")
			let _ = schemaInstance.validate(uniquenessRule: result.uniquenessRule)
		}
	}
	
	override func didValidateWhereRule(for complexEntity: SDAI.ComplexEntity, result: [SDAI.EntityReference : [SDAI.WhereLabel : SDAI.LOGICAL]]) {
		complexValidated += 1
		if let marker = progressMarker(completed: complexValidated, total: complexCount) { print(marker, terminator: "") }

		var failed = false
		for (entity,entityResult) in result {
			for (label,whereResult) in entityResult {
				if whereResult == SDAI.FALSE {
					failed = true
					let _ = type(of: entity).validateWhereRules(instance: entity, prefix: "again", round: SDAI.notValidatedYet)
				}
			}
		}
		if failed { 
			print("X", terminator: "") 
			
		}
		
	}
	
}
