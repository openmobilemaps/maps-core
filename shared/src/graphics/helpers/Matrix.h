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
 * matrices and vectors stored in float arrays.
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

class Matrix {
  public:
    static void transposeM(std::vector<float> &mTrans, int mTransOffset, std::vector<float> &m, int mOffset);
    static bool invertM(std::vector<float> &mInv, int mInvOffset, std::vector<float> &m, int mOffset);
    static void orthoM(std::vector<float> &m, int mOffset, float left, float right, float bottom, float top, float near, float far);
    static void frustumM(std::vector<float> &m, int offset, float left, float right, float bottom, float top, float near,
                         float far);
    static void perspectiveM(std::vector<float> &m, int offset, float fovy, float aspect, float zNear, float zFar);
    static float length(float x, float y, float z);
    static void setIdentityM(std::vector<float> &sm, int smOffset);
    static void setLookAtM(std::vector<float> &rm, int rmOffset, float eyeX, float eyeY, float eyeZ, float centerX, float centerY,
                           float centerZ, float upX, float upY, float upZ);

    static void scaleM(std::vector<float> &sm, int smOffset, std::vector<float> &m, int mOffset, float x, float y, float z);
    static void scaleM(std::vector<float> &m, int mOffset, float x, float y, float z);
    static void translateM(std::vector<float> &tm, int tmOffset, std::vector<float> &m, int mOffset, float x, float y, float z);
    static void translateM(std::vector<float> &m, int mOffset, float x, float y, float z);

    static void rotateM(std::vector<float> &m, int mOffset, float a, float x, float y, float z);
    static void setRotateM(std::vector<float> &rm, int rmOffset, float a, float x, float y, float z);
    static void multiplyMM(std::vector<float> &r, int resultOffset, std::vector<float> &lhs, int lhsOffset, std::vector<float> &rhs,
                           int rhsOffset);

    static std::vector<float> multiply(const std::vector<float> &M, const std::vector<float> &x);

    static std::vector<float> sTemp;
};
