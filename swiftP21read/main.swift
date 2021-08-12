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
let url = URL(fileURLWithPath: testDataFolder + "NIST_CTC_STEP_PMI/nist_ctc_02_asme1_ap242-e2.stp")

let stepsource = try String(contentsOf: url) 

var charstream = stepsource.makeIterator()

let repository = SDAISessionSchema.SdaiRepository(name: "examle", description: "example repository")

let schemaList: P21Decode.SchemaList = [
	"AP242_MANAGED_MODEL_BASED_3D_ENGINEERING_MIM_LF { 1 0 10303 442 3 1 4 }": AP242_MANAGED_MODEL_BASED_3D_ENGINEERING_MIM_LF.self,
	"AP242_MANAGED_MODEL_BASED_3D_ENGINEERING_MIM_LF { 1 0 10303 442 1 1 4 }": AP242_MANAGED_MODEL_BASED_3D_ENGINEERING_MIM_LF.self,
	"AP203_CONFIGURATION_CONTROLLED_3D_DESIGN_OF_MECHANICAL_PARTS_AND_ASSEMBLIES_MIM_LF  { 1 0 10303 403 3 1 4}": AP242_MANAGED_MODEL_BASED_3D_ENGINEERING_MIM_LF.self,
	"AP203_CONFIGURATION_CONTROLLED_3D_DESIGN_OF_MECHANICAL_PARTS_AND_ASSEMBLIES_MIM_LF { 1 0 10303 403 2 1 2}": AP242_MANAGED_MODEL_BASED_3D_ENGINEERING_MIM_LF.self,
	"CONFIG_CONTROL_DESIGN": AP242_MANAGED_MODEL_BASED_3D_ENGINEERING_MIM_LF.self,
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

//MARK: find top level entities
let exchange = decoder.exchangeStructrure!
let toplevel = exchange.topLevelEntities
let numToplevel = toplevel.count
let numTotal = exchange.entityInstanceRegistory.count
print("decoded \(numTotal) entities")
print("top level entities (\(numToplevel)): \(toplevel)\n")

var printTopLevels = false
if printTopLevels {
	for (name,instances) in toplevel {
		if let some = instances.first, let instance = exchange.entityInstanceRegistory[some], let complex = instance.resolved {
			print("\(name) #\(some): \(complex)")
			print("")
		}
	}
}

//MARK: validation
guard let schema = exchange.shcemaRegistory.values.first else { exit(3) }
let schemaInstance = SDAIPopulationSchema.SchemaInstance(repository: repository, 
																												 name: "examle", 
																												 schema: schema.schemaDefinition)
for model in createdModels {
	schemaInstance.add(model:model)
}
schemaInstance.mode = .readOnly

let validationMonitor = MyValidationMonitor()

var doIndividualWhereValidation = true
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

var doGlobalRuleValidation = true
if doGlobalRuleValidation {
	let globalResult = schemaInstance.validateGlobalRules(monitor:validationMonitor)
	print("\n glovalRuleValidationRecord(\(globalResult.count)):\n\(globalResult)"  )
}

var doUniqunessRuleValidation = true
if doUniqunessRuleValidation {
	let uniquenessResult = schemaInstance.validateUniquenessRules(monitor:validationMonitor)
	print("\n uniquenessRuleValidationRecord(\(uniquenessResult.count)):\n\(uniquenessResult)")
}	

var doWhereRuleValidation = true
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
var entityType = ap242.ePRODUCT.self
let instances = schemaInstance.entityExtent(type: entityType)
print("\(entityType): \(Array(instances))")

var name = 1
while name != 0 {
	if let instance = exchange.entityInstanceRegistory[name], let complex = instance.resolved {
		print("#\(name): source = \(instance.source)\n\(complex)")
		print("")
	}
	name = 0
	print("")
}


print("normal end of execution")


