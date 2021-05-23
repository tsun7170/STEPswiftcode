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

let monitor = MyActivityMonitor()

guard let decoder = P21Decode.Decoder(output: repository, schemaList: schemaList, monitor: monitor)
else {
	print("decoder initialization error")
	exit(1)
}
let output = decoder.decode(input: charstream)
if output == nil {
	print("decoder error: \(String(describing: decoder.error))")
}
else {
	let exchange = decoder.exchangeStructrure!
	let toplevel = exchange.topLevelEntities
	let numToplevel = toplevel.count
	let numTotal = exchange.entityInstanceRegistory.count
	print("decoded \(numTotal) entities")
	print("top level entities (\(numToplevel)): \(toplevel)")
	print("normal end of execution")
}


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
