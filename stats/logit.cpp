/*
  Copyright (C) 2005-2018 Steven L. Scott

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "stats/logit.hpp"
#include "cpputil/report_error.hpp"

namespace BOOM {

  Vector multinomial_logit(const Vector &distribution) {
    double total = sum(distribution);
    if (fabs(total - 1.0) > 1e-8) {
      report_error("Argument must sum to 1.");
    }
    Vector ans(distribution.size() - 1);
    for (int i = 0; i < ans.size(); ++i) {
      ans[i] = log(distribution[i] / distribution.back());
    }
    return ans;
  }

  Vector multinomial_logit_inverse(const Vector &logits) {
    double max_logit = std::max<double>(0.0, max(logits));
    Vector ans(logits.size() + 1);
    double total = 0;
    for (int i = 0; i < logits.size(); ++i) {
      ans[i] = exp(logits[i] - max_logit);
      total += ans[i];
    }
    double last = exp(-max_logit);
    ans.back() = last;
    total += last;
    ans /= total;
    return ans;
  }
  
}  // namespace BOOM
