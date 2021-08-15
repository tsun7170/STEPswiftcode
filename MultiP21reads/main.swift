//
//  main.swift
//  MultiP21reads
//
//  Created by Yoshida on 2021/08/14.
//  Copyright © 2021 Tsutomu Yoshida, Minokamo, Japan. All rights reserved.
//

import Foundation
import SwiftSDAIcore
import SwiftSDAIap242

print("MultiP21reads")

let testDataFolder = ProcessInfo.processInfo.environment["TEST_DATA_FOLDER"]!
//let url = URL(fileURLWithPath: testDataFolder + "CAx STEP FILE LIBRARY/s1-c5-214/s1-c5-214.stp")
let url = URL(fileURLWithPath: testDataFolder + "CAx STEP FILE LIBRARY/s1-tu-203/s1-tu-203.stp")

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

let monitor = MyActivityMonitor()

//MARK: manager creation
guard let manager = AP242P21DecodeManager(repository: repository, 
																		schemaList: schemaList, 
																		masterFile: url, 
																		monitor: monitor)
else {
	print("failed to create AP242P21DecodeManager")
	exit(1)
}

//MARK: decode
manager.decode()

print("\n LOADED p21 FILES")
for (i,entry) in manager.p21entries.values
	.sorted(by:{$0.entryName < $1.entryName})
	.enumerated() {
	print("[\(i)]\t \(entry.entryName)\t\t MODELs=\(entry.sdaiModels?.compactMap{$0.name} ?? [])\t\t STATUS= \(entry.status)", terminator:"")
	if let extref = entry as? AP242P21DecodeManager.ExternalReferenceEntry {
		print("\t UPSTREAM= \(extref.upStream.entryName)\t FORMAT= \(extref.documentFormat.asSwiftType))")
//		print("\t\t IDENT=\(String(describing: extref.loadedIdentity))")
	}
	else {
		print("")
	}
}

let schemaInstance = repository
	.createSchemaInstance(name: url.lastPathComponent, 
												schema: ap242.schemaDefinition)
schemaInstance.add(models: manager.models)
schemaInstance.mode = .readOnly

//MARK: validation
var doAllValidaton = false
if doAllValidaton {
	let validationPassed = schemaInstance.validateAllConstraints(monitor: MyValidationMonitor())
	print("\n\n validationPassed: ", validationPassed)
	print("\n glovalRuleValidationRecord: \(String(describing: schemaInstance.globalRuleValidationRecord))"  )
	print("\n uniquenessRuleValidationRecord: \(String(describing: schemaInstance.uniquenessRuleValidationRecord))")
	print("\n whereRuleValidationRecord: \(String(describing: schemaInstance.whereRuleValidationRecord))" )
}


//MARK: entity look up
var filename = url.lastPathComponent
var name = 1
while name != 0 {
	guard let exchange = manager.p21entries[filename]?.exchangeStructure 
	else {
		filename = url.lastPathComponent
		name = 0
		continue
	}
	if let instance = exchange.entityInstanceRegistory[name], let complex = instance.resolved {
		print("\n#\(name): source = \(instance.source)\n\(complex)\n")
		name = 0
		continue
	}
	else {
		name = 0
		continue
	}
}


print("normal end of execution")




