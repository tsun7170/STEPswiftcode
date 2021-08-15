//
//  swiftP21readTest.swift
//  swiftP21readTest
//
//  Created by Yoshida on 2021/06/30.
//  Copyright © 2021 Tsutomu Yoshida, Minokamo, Japan. All rights reserved.
//

import XCTest

class swiftP21readTest: XCTestCase {

	private func progressMarker(completed: Int, total: Int) -> String? {
		if total == 0 { return nil }
		if total <= 100 { return completed % 10 == 0 ? "+" : "." }
		
		if (completed * 10) % total < 10 {
			return "\((completed * 10) / total)"
		}
		if (completed * 20) % total < 20 {
			return "+"
		}
		if ( completed * 100) % total < 100 {
			return "."
		}
		return nil
	}

	func testProgressMarker() {
		var markers = ""
		let total = 22390
		for i in 1 ... total {
			if let marker = progressMarker(completed: i, total: total) {
				markers += marker
			}
		}
		
		XCTAssertTrue(markers.count == 101)
	}
	
	func testURL() {
		let testDataFolder = ProcessInfo.processInfo.environment["TEST_DATA_FOLDER"]!
		let url = URL(fileURLWithPath: testDataFolder + "CAx STEP FILE LIBRARY/s1-c5-214/s1-c5-214.stp")
		let file = url.lastPathComponent
		let dir = url.deletingLastPathComponent()
		XCTAssertTrue(file == "s1-c5-214.stp")

		let anotherUrl = URL(fileURLWithPath: dir.path+"/"+file)
		XCTAssertTrue(url == anotherUrl)
	}
}
