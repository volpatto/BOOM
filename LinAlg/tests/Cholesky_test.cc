#include "gtest/gtest.h"
#include "distributions.hpp"
#include "LinAlg/Cholesky.hpp"
#include "LinAlg/SpdMatrix.hpp"
#include "test_utils/test_utils.hpp"
#include <fstream>

namespace {
  using namespace BOOM;
  using std::endl;
  using std::cout;
  
  class CholeskyTest : public ::testing::Test {
   protected:
    CholeskyTest()
        : spd_(4)
    {
      GlobalRng::rng.seed(8675309);
      spd_.randomize();
    }
    SpdMatrix spd_;
  };

  TEST_F(CholeskyTest, WholeClass) {
    SpdMatrix spd(4);
    spd.randomize();

    Chol cholesky(spd);
    EXPECT_EQ(4, cholesky.nrow());
    EXPECT_EQ(4, cholesky.ncol());
    EXPECT_EQ(4, cholesky.dim());

    Matrix L = cholesky.getL();
    Matrix LT = cholesky.getLT();

    EXPECT_TRUE(MatrixEquals(L.transpose(), LT));
    EXPECT_TRUE(MatrixEquals(spd, L * LT))
        << "original matrix = " << endl << spd
        << "recovered matrix = " << endl << L * LT << endl;

    SpdMatrix spd_inverse = cholesky.inv();
    EXPECT_TRUE(MatrixEquals(spd * spd_inverse, SpdMatrix(4, 1.0)));

    Vector v(4);
    v.randomize();

    Vector x = cholesky.solve(v);
    EXPECT_TRUE(VectorEquals(spd * x, v));

    EXPECT_TRUE(MatrixEquals(spd, cholesky.original_matrix()))
        << "Original matrix = " << endl << spd << endl
        << "Recovered matrix = " << endl << cholesky.original_matrix() << endl;
    
    EXPECT_NEAR(cholesky.logdet(),
                log(fabs(spd.Matrix::det())),
                1e-8);

    EXPECT_NEAR(cholesky.det(),
                fabs(spd.Matrix::det()),
                1e-8);
  }

  // This test is identical to the WholeClass test except the matrix is
  // decomposed after the cholesky object is constructe
  TEST_F(CholeskyTest, DecomposeLater) {
    SpdMatrix spd(4);
    spd.randomize();

    Chol cholesky;
    cholesky.decompose(spd);

    EXPECT_EQ(4, cholesky.nrow());
    EXPECT_EQ(4, cholesky.ncol());
    EXPECT_EQ(4, cholesky.dim());

    Matrix L = cholesky.getL();
    Matrix LT = cholesky.getLT();

    EXPECT_TRUE(MatrixEquals(L.transpose(), LT));
    EXPECT_TRUE(MatrixEquals(spd, L * LT))
        << "original matrix = " << endl << spd
        << "recovered matrix = " << endl << L * LT << endl;

    SpdMatrix spd_inverse = cholesky.inv();
    EXPECT_TRUE(MatrixEquals(spd * spd_inverse, SpdMatrix(4, 1.0)));

    Vector v(4);
    v.randomize();

    Vector x = cholesky.solve(v);
    EXPECT_TRUE(VectorEquals(spd * x, v));

    EXPECT_TRUE(MatrixEquals(spd, cholesky.original_matrix()))
        << "Original matrix = " << endl << spd << endl
        << "Recovered matrix = " << endl << cholesky.original_matrix() << endl;
    
    EXPECT_NEAR(cholesky.logdet(),
                log(fabs(spd.Matrix::det())),
                1e-8);

    EXPECT_NEAR(cholesky.det(),
                fabs(spd.Matrix::det()),
                1e-8);
  }

  // If a matrix is rank deficient then the Cholesky decomposition should still
  // produce a lower triangular matrix that multiplies to the original matrix.
  TEST_F(CholeskyTest, RankDeficient) {
    int dim = 10;
    SpdMatrix deficient(dim);
    for (int i = 0; i < 3; ++i) {
      Vector x(dim);
      x.randomize();
      deficient.add_outer(x);
    }
    Chol cholesky(deficient);
    SpdMatrix deficient_copy = cholesky.original_matrix();
    EXPECT_TRUE(MatrixEquals(deficient, deficient_copy))
        << "deficient:" << endl << deficient
        << "reconstructed: " << endl << deficient_copy;
  }
  
  
}  // namespace
