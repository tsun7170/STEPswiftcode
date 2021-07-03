//
//  swiftP21readTest.swift
//  swiftP21readTest
//
//  Created by Yoshida on 2021/06/30.
//  Copyright © 2021 Minokamo, Japan. All rights reserved.
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
	

}
