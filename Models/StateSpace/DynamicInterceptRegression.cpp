// Copyright 2018 Google LLC. All Rights Reserved.
/*
  Copyright (C) 2005-2017 Steven L. Scott

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

#include "Models/StateSpace/DynamicInterceptRegression.hpp"
#include "distributions.hpp"

namespace BOOM {

  namespace {
    using DIRM = DynamicInterceptRegressionModel;
    using StateSpace::TimeSeriesRegressionData;
  }  // namespace

  namespace StateSpace {
    TimeSeriesRegressionData::TimeSeriesRegressionData(
        const Vector &response,
        const Matrix &predictors)
        : response_(response),
          predictors_(predictors),
          observed_(response_.size(), true)
    {
      if (response_.size() != predictors_.nrow()) {
        report_error("Different numbers of observations in 'response' "
                     "and 'predictors'.");
      }
      for (int i = 0; i < response_.size(); ++i) {
        regression_data_.push_back(new RegressionData(
            response_[i], predictors_.row(i)));
      }
    }

    TimeSeriesRegressionData * TimeSeriesRegressionData::clone() const {
      return new TimeSeriesRegressionData(*this);
    }

    std::ostream &TimeSeriesRegressionData::display(std::ostream &out) const {
      out << cbind(response_, predictors_);
      return out;
    }
    
  }  // namespace StateSpace

  //===========================================================================
  
  DIRM::DynamicInterceptRegressionModel(int xdim) {
    initialize_regression_component(xdim);
  }

  DIRM::DynamicInterceptRegressionModel(const DIRM &rhs)
      : ConditionalIidMultivariateStateSpaceModelBase(rhs) {
    initialize_regression_component(rhs.xdim());
    regression_->regression()->set_Beta(rhs.regression_->regression()->Beta());
    regression_->regression()->set_sigsq(
        rhs.regression_->regression()->sigsq());
  }

  RegressionModel *DIRM::observation_model() {
    return regression_->regression();
  }

  const RegressionModel *DIRM::observation_model() const {
    return regression_->regression();
  }

  void DIRM::observe_data_given_state(int t) {
    if (!is_missing_observation(t)) {
      // Unless the data point is completely missing, add the regression
      // component of its data to the regression model.  We will do this by
      // subtracting the state mean from the y value of each observation.  The
      // state mean contains contribution from all state elements, including the
      // regression, so we need to add the regression component back in.
      Ptr<TimeSeriesRegressionData> data(dat()[t]);
      Vector state_contribution = (*observation_coefficients(t)) * state(t);
      RegressionModel *regression = regression_->regression();
      for (int i = 0; i < data->sample_size(); ++i) {
        const Ptr<RegressionData> &data_point(data->regression_data(i));
        double regression_prediction = regression->predict(data_point->x());
        observation_model()->suf()->add_mixture_data(
            data_point->y() - state_contribution[i] + regression_prediction,
            data_point->x(), 1.0);
      }
    }
  }

  void DIRM::impute_state(RNG &rng) {
    StateSpaceModelBase::impute_state(rng);
    observation_model()->suf()->fix_xtx();
  }
  
  void DIRM::add_data(const Ptr<Data> &dp) { add_data(DAT(dp)); }
  void DIRM::add_data(TimeSeriesRegressionData *dp) {
    add_data(Ptr<TimeSeriesRegressionData>(dp));
  }
  void DIRM::add_data(const Ptr<TimeSeriesRegressionData> &dp) {
    for (int i = 0; i < dp->sample_size(); ++i) {
      regression_->regression()->add_data(dp->regression_data(i));
    }
    DataPolicy::add_data(dp);
  }

  const SparseKalmanMatrix *DIRM::observation_coefficients(int t) const {
    observation_coefficients_.clear();
    const StateSpace::TimeSeriesRegressionData &data_point(*dat()[t]);
    for (int s = 0; s < number_of_state_models(); ++s) {
      observation_coefficients_.add_block(
          state_model(s)->dynamic_intercept_regression_observation_coefficients(
              t, data_point));
    }
    return &observation_coefficients_;
  }

  double DIRM::observation_variance(int t) const {
    return regression_->regression()->sigsq();
  }

  const Vector &DIRM::observation(int t) const {
    return dat()[t]->response();
  }

  Vector DIRM::conditional_mean(int time) const {
    return (*observation_coefficients(time) * state().col(time));
  }

  //===========================================================================
  // protected:
  void DIRM::observe_state(int t) {
    if (t == 0) {
      observe_initial_state();
      return;
    }
    const ConstVectorView now(state().col(t));
    const ConstVectorView then(state().col(t - 1));
    for (int s = 0; s < number_of_state_models(); ++s) {
      report_error(
          "Need to implement observe_dynamic_regression_state in all "
          "StateModels.");
      state_model(s)->observe_dynamic_intercept_regression_state(
          state_component(then, s), state_component(now, s), t, this);
    }
  }

  //===========================================================================
  // private:
  Vector DIRM::simulate_observation(RNG &rng, int t) {
    Vector ans = (*observation_coefficients(t)) * state(t);
    double residual_sd = sqrt(observation_variance(t));
    for (int i = 0; i < ans.size(); ++i) {
      ans[i] += rnorm_mt(rng, 0, residual_sd);
    }
    return ans;
  }

  void DIRM::initialize_regression_component(int xdim) {
    regression_.reset(new RegressionStateModel(new RegressionModel(xdim)));
    add_state(regression_);
  }

}  // namespace BOOM
