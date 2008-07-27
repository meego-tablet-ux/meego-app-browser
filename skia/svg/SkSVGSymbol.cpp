/* libs/graphics/svg/SkSVGSymbol.cpp
**
** Copyright 2006, Google Inc.
**
** Licensed under the Apache License, Version 2.0 (the "License"); 
** you may not use this file except in compliance with the License. 
** You may obtain a copy of the License at 
**
**     http://www.apache.org/licenses/LICENSE-2.0 
**
** Unless required by applicable law or agreed to in writing, software 
** distributed under the License is distributed on an "AS IS" BASIS, 
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
** See the License for the specific language governing permissions and 
** limitations under the License.
*/

#include "SkSVGSymbol.h"
#include "SkSVGParser.h"

const SkSVGAttribute SkSVGSymbol::gAttributes[] = {
    SVG_ATTRIBUTE(viewBox)
};

DEFINE_SVG_INFO(Symbol)

void SkSVGSymbol::translate(SkSVGParser& parser, bool defState) {
    INHERITED::translate(parser, defState);
    // !!! children need to be written into document 
}
