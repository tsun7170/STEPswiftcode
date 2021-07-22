//
//  builtin.h
//  STEPswiftcode
//
//  Created by Yoshida on 2020/09/21.
//  Copyright © 2020 Tsutomu Yoshida, Minokamo, Japan. All rights reserved.
//

#ifndef builtin_h
#define builtin_h

#include "alg.h"

//*TY2020/09/20
extern Function
func_abs,   func_acos,  func_asin,  func_atan,
						func_blength,
						func_cos,   func_exists,    func_exp,   func_format,
						func_hibound,   func_hiindex,   func_length,    func_lobound,
						func_log,   func_log10, func_log2,  func_loindex,
						func_odd,   func_rolesof,   func_sin,   func_sizeof,
						func_sqrt,  func_tan,   func_typeof,
						func_value, func_value_in,  func_value_unique;
extern Procedure
proc_insert,    proc_remove;




#endif /* builtin_h */
