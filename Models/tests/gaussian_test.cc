#include "gtest/gtest.h"
#include "Models/GaussianModel.hpp"
#include "Models/ChisqModel.hpp"
#include "distributions.hpp"
#include "cpputil/lse.hpp"
#include "numopt/NumericalDerivatives.hpp"
#include "numopt/Integral.hpp"

#include "test_utils/test_utils.hpp"
#include <fstream>

namespace {
  using namespace BOOM;
  using std::endl;
  using std::cout;

  class GaussianTest : public ::testing::Test {
   protected:
    GaussianTest() {
      GlobalRng::rng.seed(8675309);
    }
  };

  TEST_F(GaussianTest, Suf) {
    int nobs = 5;
    Vector y(nobs);
    GaussianSuf suf;
    EXPECT_DOUBLE_EQ(0, suf.n());
    EXPECT_DOUBLE_EQ(0, suf.sum());
    EXPECT_DOUBLE_EQ(0, suf.ybar());
    EXPECT_DOUBLE_EQ(0, suf.sumsq());
    EXPECT_DOUBLE_EQ(0, suf.sample_var());

    for (int i = 0; i < nobs; ++i) {
      y[i] = rnorm(3, 7);
    }
    suf.update_raw(y[0]);
    EXPECT_DOUBLE_EQ(1.0, suf.n());
    EXPECT_DOUBLE_EQ(y[0], suf.sum());
    EXPECT_DOUBLE_EQ(square(y[0]), suf.sumsq());
    EXPECT_DOUBLE_EQ(0, suf.centered_sumsq(suf.ybar()));
  }

  TEST_F(GaussianTest, LogLikelihood) {
    int nobs = 4;
    Vector y(nobs);
    GaussianModel model;
    for (int j = 0; j < nobs; ++j) {
      y[j] = rnorm(8, 2);
      model.suf()->update_raw(y[j]);
    }

    model.set_mu(3);
    model.set_sigsq(7 * 7);
    
    double loglike_manual = 0;
    for (int j = 0; j < nobs; ++j) {
      loglike_manual += dnorm(y[j], 3, 7, true);
    } 

    EXPECT_NEAR(loglike_manual,
                model.loglike(Vector{3, 7*7}),
                1e-8);

    NumericalDerivatives derivs(
        [&model](const Vector &mu_sigsq) { return model.loglike(mu_sigsq); });

    Vector test_values = {5.0, 2.8};
    Vector analytic_gradient(2);
    Matrix analytic_hessian(2, 2);
    double loglike = model.Loglike(test_values, analytic_gradient, analytic_hessian, 2);

    EXPECT_TRUE(VectorEquals(analytic_gradient, derivs.gradient(test_values)));
    EXPECT_TRUE(MatrixEquals(analytic_hessian, derivs.Hessian(test_values), 1e-3))
        << endl
        << "Analytic Hessian: " << endl
        << analytic_hessian << endl
        << "Numeric Hessian: " << endl
        << derivs.Hessian(test_values);

    loglike_manual = 0;
    for (int i = 0; i < y.size(); ++i) {
      loglike_manual += dnorm(y[i], test_values[0], sqrt(test_values[1]), true);
    }
    EXPECT_NEAR(loglike_manual, loglike, 1e-8);
  }


  // Verify that the log integrated likelihood with respect to mu (conditional
  // on sigma) matches the result returned by quadrature.
  TEST_F(GaussianTest, log_integrated_likelihood_mu) {
    int nobs = 4;
    Vector y(nobs);
    GaussianSuf suf;
    double sigma = 2;
    for (int j = 0; j < nobs; ++j) {
      y[j] = rnorm(8, sigma);
      suf.update_raw(y[j]);
    }

    double mu0 = 10;
    double tausq = 4.0;
    Integral integral([y, sigma, mu0, tausq](double mu) {
        double ans = dnorm(mu, mu0, sqrt(tausq), false);
        for (int i = 0; i < y.size(); ++i) {
          ans *= dnorm(y[i], mu, sigma, false);
        }
        return ans;
      });
    double quadrature_ans = log(integral.integrate());
    
    double ans = GaussianModelBase::log_integrated_likelihood(
        suf, mu0, tausq, square(sigma));
    EXPECT_NEAR(quadrature_ans, ans, 1e-4);
  }

  TEST_F(GaussianTest, log_integrated_likelihood_mu_sigsq) {
    int nobs = 4;
    Vector y(nobs);
    GaussianSuf suf;
    for (int j = 0; j < nobs; ++j) {
      y[j] = rnorm(8, 2);
      suf.update_raw(y[j]);
    }
    double mu0 = 10;
    double kappa = 2;
    double df = 1;
    double ss = 2;

    // True precision is 1 / 2^2 = 0.25.
    
    Integral integral(
        [suf, mu0, kappa, df, ss](double precision) {
          double sigsq = 1.0 / precision;
          double tausq = sigsq / kappa;
          return dgamma(precision, df / 2, ss / 2)
              * exp(GaussianModelBase::log_integrated_likelihood(
                  suf, mu0, tausq, sigsq));
        },
        0, 20);
    double quadrature_ans = log(integral.integrate());
    double ans = GaussianModelBase::log_integrated_likelihood(
        suf, mu0, kappa, df, ss);
    EXPECT_NEAR(quadrature_ans, ans, 1e-4);
  }  
  
}  // namespace
