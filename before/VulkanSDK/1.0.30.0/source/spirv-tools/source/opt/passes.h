// Copyright (c) 2016 Google Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and/or associated documentation files (the
// "Materials"), to deal in the Materials without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Materials, and to
// permit persons to whom the Materials are furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Materials.
//
// MODIFICATIONS TO THIS FILE MAY MEAN IT NO LONGER ACCURATELY REFLECTS
// KHRONOS STANDARDS. THE UNMODIFIED, NORMATIVE VERSIONS OF KHRONOS
// SPECIFICATIONS AND HEADER INFORMATION ARE LOCATED AT
//    https://www.khronos.org/registry/
//
// THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.

#ifndef LIBSPIRV_OPT_PASSES_H_
#define LIBSPIRV_OPT_PASSES_H_

#include <memory>

#include "module.h"
#include "pass.h"

namespace spvtools {
namespace opt {

// A null pass that does nothing.
class NullPass : public Pass {
  const char* name() const override { return "null"; }
  bool Process(ir::Module*) override { return false; }
};

// The optimization pass for removing debug instructions (as documented in
// Section 3.32.2 of the SPIR-V spec).
class StripDebugInfoPass : public Pass {
 public:
  const char* name() const override { return "strip-debug"; }
  bool Process(ir::Module* module) override;
};

// The transformation pass that specializes the value of spec constants to
// their default values. This pass only processes the spec constants that have
// Spec ID decorations (defined by OpSpecConstant, OpSpecConstantTrue and
// OpSpecConstantFalse instructions) and replaces them with their front-end
// version counterparts (OpConstant, OpConstantTrue and OpConstantFalse). The
// corresponding Spec ID annotation instructions will also be removed. This
// pass does not fold the newly added front-end constants and does not process
// other spec constants defined by OpSpecConstantComposite or OpSpecConstantOp.
class FreezeSpecConstantValuePass : public Pass {
 public:
  const char* name() const override { return "freeze-spec-const"; }
  bool Process(ir::Module*) override;
};

// The optimization pass to remove dead constants, including front-end
// contants: defined by OpConstant, OpConstantComposite, OpConstantTrue and
// OpConstantFalse; and spec constants: defined by OpSpecConstant,
// OpSpecConstantComposite, OpSpecConstantTrue, OpSpecConstantFalse and
// OpSpecConstantOp.
class EliminateDeadConstantPass : public Pass {
 public:
  const char* name() const override { return "eliminate-dead-const"; }
  bool Process(ir::Module*) override;
};

}  // namespace opt
}  // namespace spvtools

#endif  // LIBSPIRV_OPT_PASSES_H_
