//
//  MyValidationMonitor.swift
//  swiftP21read
//
//  Created by Yoshida on 2021/07/10.
//  Copyright © 2021 Tsutomu Yoshida, Minokamo, Japan. All rights reserved.
//

import Foundation
import SwiftSDAIcore

//MARK: - validation monitor
class MyValidationMonitor: SDAIPopulationSchema.ValidationMonitor {
	var globalCount: Int = 0
	var uniquenessCount: Int = 0
	var complexCount: Int = 0
	
	var globalValidated: Int = 0
	var uniquenessValidated: Int = 0
	var complexValidated: Int = 0
	
	var confirmFailedCase = false
	
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
		if (completed * 20) % total < 20 {
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
			print("/", terminator: "")
			if confirmFailedCase {
				let _ = schemaInstance.validate(globalRule: result.globalRule)
			}
		}
	}
	
	override func didValidateUniquenessRule(for schemaInstance: SDAIPopulationSchema.SchemaInstance, result: SDAI.UniquenessRuleValidationResult) {
		uniquenessValidated += 1
		if let marker = progressMarker(completed: uniquenessValidated, total: uniquenessCount) { print(marker, terminator: "") }

		if result.result == SDAI.FALSE {
			print("/", terminator: "")
			if confirmFailedCase {
				let _ = schemaInstance.validate(uniquenessRule: result.uniquenessRule)
			}
		}
	}
	
	override func didValidateWhereRule(for complexEntity: SDAI.ComplexEntity, result: [SDAI.WhereLabel : SDAI.LOGICAL]) {
		complexValidated += 1
		if let marker = progressMarker(completed: complexValidated, total: complexCount) { print(marker, terminator: "") }

		var failed = false
		for (label,whereResult) in result {
			if whereResult == SDAI.FALSE {
				failed = true
				if confirmFailedCase {
					print("\nFAILED: \(label)")
					let _ = complexEntity.validateEntityWhereRules(prefix: "again", recording: .recordFailureOnly)
				}
			}
		}
		if failed { 
			print("/", terminator: "") 
			
		}
		
	}
	
}
