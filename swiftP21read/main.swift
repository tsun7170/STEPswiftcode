//
//  main.swift
//  swiftP21read
//
//  Created by Yoshida on 2020/09/09.
//  Copyright © 2020 Minokamo, Japan. All rights reserved.
//

import Foundation
import SwiftSDAIcore

print("swiftP21read")

let url = URL(fileURLWithPath: "./TestCases/NIST_CTC_STEP_PMI/nist_ctc_02_asme1_ap242-e2.stp")

let stepsource = try String(contentsOf: url) 

var charstream = stepsource.makeIterator()

//var p21stream = P21Decode.P21CharacterStream(charStream: charstream)
//
//while p21stream.lineNumber < 10 {
//	print(p21stream.next() ?? "<nil>", terminator:"")
//}

let parser = P21Decode.ExchangeStructureParser(charStream: charstream)

parser.parseExchangeStructure()


