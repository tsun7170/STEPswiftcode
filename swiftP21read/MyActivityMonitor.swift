//
//  MyActivityMonitor.swift
//  swiftP21read
//
//  Created by Yoshida on 2021/07/10.
//  Copyright © 2021 Tsutomu Yoshida, Minokamo, Japan. All rights reserved.
//

import Foundation
import SwiftSDAIcore

//MARK: - activity monitor
class MyActivityMonitor: AP242P21DecodeManager.ActivityMonitor {
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
	
	//MARK: AP242P21DecodeManager specific
	override func startedLoading(p21entry: AP242P21DecodeManager.P21Entry) {
		print("\n loading file: \(p21entry.entryName) ", terminator: "")
		entityCount = 0
	}
	
	override func completedLoading(p21entry: AP242P21DecodeManager.P21Entry) {
		print(" file: \(p21entry.entryName) completed loading with status(\(p21entry.status))")
	}
	
	override func identified(externalReferences: [AP242P21DecodeManager.ExternalReferenceEntry], originatedFrom p21entry: AP242P21DecodeManager.P21Entry) {
		print(" file: \(p21entry.entryName) contains \(externalReferences.count) external references.")
	}
}
