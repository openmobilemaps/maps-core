/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

/**
 * Matrix math utilities. These methods operate on OpenGL ES format
 * matrices and vectors stored in double arrays.
 * <p>
 * Matrices are 4 x 4 column-vector matrices stored in column-major
 * order:
 * <pre>
 *  m[offset +  0] m[offset +  4] m[offset +  8] m[offset + 12]
 *  m[offset +  1] m[offset +  5] m[offset +  9] m[offset + 13]
 *  m[offset +  2] m[offset +  6] m[offset + 10] m[offset + 14]
 *  m[offset +  3] m[offset +  7] m[offset + 11] m[offset + 15]</pre>
 *
 * Vectors are 4 x 1 column vectors stored in order:
 * <pre>
 * v[offset + 0]
 * v[offset + 1]
 * v[offset + 2]
 * v[offset + 3]</pre>
 */

#pragma once

#include <cmath>
#include <vector>
#include <string>

#include "Vec4D.h"

class MatrixD {
public:
    static void transposeM(std::vector<double> &mTrans, int mTransOffset, std::vector<double> &m, int mOffset);

    static bool invertM(std::vector<double> &mInv, int mInvOffset, std::vector<double> &m, int mOffset);

    static void orthoM(std::vector<double> &m, int mOffset, double left, double right, double bottom, double top, double near, double far);

    static void frustumM(std::vector<double> &m, int offset, double left, double right, double bottom, double top, double near,
                         double far);

    static void perspectiveM(std::vector<double> &m, int offset, double fovy, double aspect, double zNear, double zFar);

    static double length(double x, double y, double z);

    static void setIdentityM(std::vector<double> &sm, int smOffset);

    static void setLookAtM(std::vector<double> &rm, int rmOffset, double eyeX, double eyeY, double eyeZ, double centerX, double centerY,
                           double centerZ, double upX, double upY, double upZ);

    static void scaleM(std::vector<double> &sm, int smOffset, std::vector<double> &m, int mOffset, double x, double y, double z);

    static void scaleM(std::vector<double> &m, int mOffset, double x, double y, double z);

    static void translateM(std::vector<double> &tm, int tmOffset, std::vector<double> &m, int mOffset, double x, double y, double z);

    static void translateM(std::vector<double> &m, int mOffset, double x, double y, double z);

    static void mTranslated(std::vector<double> &tm, int tmOffset, std::vector<double> &m, int mOffset, double x, double y, double z);

    static void mTranslated(std::vector<double> &m, int mOffset, double x, double y, double z);

    static void rotateM(std::vector<double> &m, int mOffset, double a, double x, double y, double z);

    static void setRotateM(std::vector<double> &rm, int rmOffset, double a, double x, double y, double z);

    static void multiplyMM(std::vector<double> &r, int resultOffset, std::vector<double> &lhs, int lhsOffset, std::vector<double> &rhs,
                           int rhsOffset);

    static void multiplyMMC(std::vector<double> &r, int resultOffset, const std::vector<double> &lhs, int lhsOffset,
                            const std::vector<double> &rhs, int rhsOffset);

    static Vec4D multiply(const std::vector<double> &M, const Vec4D &x);

    static void multiply(const std::vector<double> &M, const Vec4D &x, Vec4D &result);

    static std::vector<double> sTemp;

    static std::string toMatrixString(const std::vector<double> &M);

    static std::string toVectorString(const std::vector<double> &v);
};
