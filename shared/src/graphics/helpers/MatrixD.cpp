/*
 * Copyright (c) 2021 Ubique Innovation AG <https://www.ubique.ch>
 *
 *  This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 *  file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *
 *  SPDX-License-Identifier: MPL-2.0
 */

#include "MatrixD.h"
#include <sstream>

/**
 * Transposes a 4 x 4 matrix.
 * <p>
 * mTrans and m must not overlap.
 *
 * @param mTrans the array that holds the output transposed matrix
 * @param mTransOffset an offset into mTrans where the transposed matrix is
 *        stored.
 * @param m the input array
 * @param mOffset an offset into m where the input matrix is stored.
 */
void MatrixD::transposeM(std::vector<double> &mTrans, int mTransOffset, std::vector<double> &m, int mOffset) {
    for (int i = 0; i < 4; i++) {
        int mBase = i * 4 + mOffset;
        mTrans[i + mTransOffset] = m[mBase];
        mTrans[i + 4 + mTransOffset] = m[mBase + 1];
        mTrans[i + 8 + mTransOffset] = m[mBase + 2];
        mTrans[i + 12 + mTransOffset] = m[mBase + 3];
    }
}

/**
 * Inverts a 4 x 4 matrix.
 * <p>
 * mInv and m must not overlap.
 *
 * @param mInv the array that holds the output inverted matrix
 * @param mInvOffset an offset into mInv where the inverted matrix is
 *        stored.
 * @param m the input array
 * @param mOffset an offset into m where the input matrix is stored.
 * @return true if the matrix could be inverted, false if it could not.
 */
bool MatrixD::invertM(std::vector<double> &mInv, int mInvOffset, std::vector<double> &m, int mOffset) {
    // Invert a 4 x 4 matrix using Cramer's Rule

    // transpose matrix
    double src0 = m[mOffset + 0];
    double src4 = m[mOffset + 1];
    double src8 = m[mOffset + 2];
    double src12 = m[mOffset + 3];

    double src1 = m[mOffset + 4];
    double src5 = m[mOffset + 5];
    double src9 = m[mOffset + 6];
    double src13 = m[mOffset + 7];

    double src2 = m[mOffset + 8];
    double src6 = m[mOffset + 9];
    double src10 = m[mOffset + 10];
    double src14 = m[mOffset + 11];

    double src3 = m[mOffset + 12];
    double src7 = m[mOffset + 13];
    double src11 = m[mOffset + 14];
    double src15 = m[mOffset + 15];

    // calculate pairs for first 8 elements (cofactors)
    double atmp0 = src10 * src15;
    double atmp1 = src11 * src14;
    double atmp2 = src9 * src15;
    double atmp3 = src11 * src13;
    double atmp4 = src9 * src14;
    double atmp5 = src10 * src13;
    double atmp6 = src8 * src15;
    double atmp7 = src11 * src12;
    double atmp8 = src8 * src14;
    double atmp9 = src10 * src12;
    double atmp10 = src8 * src13;
    double atmp11 = src9 * src12;

    // calculate first 8 elements (cofactors)
    double dst0 = (atmp0 * src5 + atmp3 * src6 + atmp4 * src7) - (atmp1 * src5 + atmp2 * src6 + atmp5 * src7);
    double dst1 = (atmp1 * src4 + atmp6 * src6 + atmp9 * src7) - (atmp0 * src4 + atmp7 * src6 + atmp8 * src7);
    double dst2 = (atmp2 * src4 + atmp7 * src5 + atmp10 * src7) - (atmp3 * src4 + atmp6 * src5 + atmp11 * src7);
    double dst3 = (atmp5 * src4 + atmp8 * src5 + atmp11 * src6) - (atmp4 * src4 + atmp9 * src5 + atmp10 * src6);
    double dst4 = (atmp1 * src1 + atmp2 * src2 + atmp5 * src3) - (atmp0 * src1 + atmp3 * src2 + atmp4 * src3);
    double dst5 = (atmp0 * src0 + atmp7 * src2 + atmp8 * src3) - (atmp1 * src0 + atmp6 * src2 + atmp9 * src3);
    double dst6 = (atmp3 * src0 + atmp6 * src1 + atmp11 * src3) - (atmp2 * src0 + atmp7 * src1 + atmp10 * src3);
    double dst7 = (atmp4 * src0 + atmp9 * src1 + atmp10 * src2) - (atmp5 * src0 + atmp8 * src1 + atmp11 * src2);

    // calculate pairs for second 8 elements (cofactors)
    double btmp0 = src2 * src7;
    double btmp1 = src3 * src6;
    double btmp2 = src1 * src7;
    double btmp3 = src3 * src5;
    double btmp4 = src1 * src6;
    double btmp5 = src2 * src5;
    double btmp6 = src0 * src7;
    double btmp7 = src3 * src4;
    double btmp8 = src0 * src6;
    double btmp9 = src2 * src4;
    double btmp10 = src0 * src5;
    double btmp11 = src1 * src4;

    // calculate second 8 elements (cofactors)
    double dst8 = (btmp0 * src13 + btmp3 * src14 + btmp4 * src15) - (btmp1 * src13 + btmp2 * src14 + btmp5 * src15);
    double dst9 = (btmp1 * src12 + btmp6 * src14 + btmp9 * src15) - (btmp0 * src12 + btmp7 * src14 + btmp8 * src15);
    double dst10 = (btmp2 * src12 + btmp7 * src13 + btmp10 * src15) - (btmp3 * src12 + btmp6 * src13 + btmp11 * src15);
    double dst11 = (btmp5 * src12 + btmp8 * src13 + btmp11 * src14) - (btmp4 * src12 + btmp9 * src13 + btmp10 * src14);
    double dst12 = (btmp2 * src10 + btmp5 * src11 + btmp1 * src9) - (btmp4 * src11 + btmp0 * src9 + btmp3 * src10);
    double dst13 = (btmp8 * src11 + btmp0 * src8 + btmp7 * src10) - (btmp6 * src10 + btmp9 * src11 + btmp1 * src8);
    double dst14 = (btmp6 * src9 + btmp11 * src11 + btmp3 * src8) - (btmp10 * src11 + btmp2 * src8 + btmp7 * src9);
    double dst15 = (btmp10 * src10 + btmp4 * src8 + btmp9 * src9) - (btmp8 * src9 + btmp11 * src10 + btmp5 * src8);

    // calculate determinant
    double det = src0 * dst0 + src1 * dst1 + src2 * dst2 + src3 * dst3;

    if (det == 0.0f) {
        return false;
    }

    // calculate matrix inverse
    double invdet = 1.0f / det;
    mInv[mInvOffset] = dst0 * invdet;
    mInv[1 + mInvOffset] = dst1 * invdet;
    mInv[2 + mInvOffset] = dst2 * invdet;
    mInv[3 + mInvOffset] = dst3 * invdet;

    mInv[4 + mInvOffset] = dst4 * invdet;
    mInv[5 + mInvOffset] = dst5 * invdet;
    mInv[6 + mInvOffset] = dst6 * invdet;
    mInv[7 + mInvOffset] = dst7 * invdet;

    mInv[8 + mInvOffset] = dst8 * invdet;
    mInv[9 + mInvOffset] = dst9 * invdet;
    mInv[10 + mInvOffset] = dst10 * invdet;
    mInv[11 + mInvOffset] = dst11 * invdet;

    mInv[12 + mInvOffset] = dst12 * invdet;
    mInv[13 + mInvOffset] = dst13 * invdet;
    mInv[14 + mInvOffset] = dst14 * invdet;
    mInv[15 + mInvOffset] = dst15 * invdet;

    return true;
}

/**
 * Computes an orthographic projection matrix.
 *
 * @param m returns the result
 */
void MatrixD::orthoM(std::vector<double> &m, int mOffset, double left, double right, double bottom, double top, double near, double far) {
    double r_width = 1.0f / (right - left);
    double r_height = 1.0f / (top - bottom);
    double r_depth = 1.0f / (far - near);
    double x = 2.0f * (r_width);
    double y = 2.0f * (r_height);
    double z = -2.0f * (r_depth);
    double tx = -(right + left) * r_width;
    double ty = -(top + bottom) * r_height;
    double tz = -(far + near) * r_depth;
    m[mOffset + 0] = x;
    m[mOffset + 5] = y;
    m[mOffset + 10] = z;
    m[mOffset + 12] = tx;
    m[mOffset + 13] = ty;
    m[mOffset + 14] = tz;
    m[mOffset + 15] = 1.0f;
    m[mOffset + 1] = 0.0f;
    m[mOffset + 2] = 0.0f;
    m[mOffset + 3] = 0.0f;
    m[mOffset + 4] = 0.0f;
    m[mOffset + 6] = 0.0f;
    m[mOffset + 7] = 0.0f;
    m[mOffset + 8] = 0.0f;
    m[mOffset + 9] = 0.0f;
    m[mOffset + 11] = 0.0f;
}

/**
 * Defines a projection matrix in terms of six clip planes.
 *
 * @param m the double array that holds the output perspective matrix
 * @param offset the offset into double array m where the perspective
 *        matrix data is written
 */
void MatrixD::frustumM(std::vector<double> &m, int offset, double left, double right, double bottom, double top, double near, double far) {
    double r_width = 1.0f / (right - left);
    double r_height = 1.0f / (top - bottom);
    double r_depth = 1.0f / (near - far);
    double x = 2.0f * (near * r_width);
    double y = 2.0f * (near * r_height);
    double A = (right + left) * r_width;
    double B = (top + bottom) * r_height;
    double C = (far + near) * r_depth;
    double D = 2.0f * (far * near * r_depth);
    m[offset + 0] = x;
    m[offset + 5] = y;
    m[offset + 8] = A;
    m[offset + 9] = B;
    m[offset + 10] = C;
    m[offset + 14] = D;
    m[offset + 11] = -1.0f;
    m[offset + 1] = 0.0f;
    m[offset + 2] = 0.0f;
    m[offset + 3] = 0.0f;
    m[offset + 4] = 0.0f;
    m[offset + 6] = 0.0f;
    m[offset + 7] = 0.0f;
    m[offset + 12] = 0.0f;
    m[offset + 13] = 0.0f;
    m[offset + 15] = 0.0f;
}

/**
 * Defines a projection matrix in terms of a field of view angle, an
 * aspect ratio, and z clip planes.
 *
 * @param m the double array that holds the perspective matrix
 * @param offset the offset into double array m where the perspective
 *        matrix data is written
 * @param fovy field of view in y direction, in degrees
 * @param aspect width to height aspect ratio of the viewport
 */
void MatrixD::perspectiveM(std::vector<double> &m, int offset, double fovy, double aspect, double zNear, double zFar) {
    double f = 1.0f / (double)std::tan(fovy * (M_PI / 360.0));
    double rangeReciprocal = 1.0f / (zNear - zFar);

    m[offset + 0] = f / aspect;
    m[offset + 1] = 0.0f;
    m[offset + 2] = 0.0f;
    m[offset + 3] = 0.0f;

    m[offset + 4] = 0.0f;
    m[offset + 5] = f;
    m[offset + 6] = 0.0f;
    m[offset + 7] = 0.0f;

    m[offset + 8] = 0.0f;
    m[offset + 9] = 0.0f;
    m[offset + 10] = (zFar + zNear) * rangeReciprocal;
    m[offset + 11] = -1.0f;

    m[offset + 12] = 0.0f;
    m[offset + 13] = 0.0f;
    m[offset + 14] = 2.0f * zFar * zNear * rangeReciprocal;
    m[offset + 15] = 0.0f;
}

/**
 * Computes the length of a vector.
 *
 * @param x x coordinate of a vector
 * @param y y coordinate of a vector
 * @param z z coordinate of a vector
 * @return the length of a vector
 */
double MatrixD::length(double x, double y, double z) { return (double)std::sqrt(x * x + y * y + z * z); }

/**
 * Sets matrix m to the identity matrix.
 *
 * @param sm returns the result
 * @param smOffset index into sm where the result matrix starts
 */
void MatrixD::setIdentityM(std::vector<double> &sm, int smOffset) {
    for (int i = 0; i < 16; i++) {
        sm[smOffset + i] = 0;
    }
    for (int i = 0; i < 16; i += 5) {
        sm[smOffset + i] = 1.0f;
    }
}

/**
 * Scales matrix m by x, y, and z, putting the result in sm.
 * <p>
 * m and sm must not overlap.
 *
 * @param sm returns the result
 * @param smOffset index into sm where the result matrix starts
 * @param m source matrix
 * @param mOffset index into m where the source matrix starts
 * @param x zoom factor x
 * @param y zoom factor y
 * @param z zoom factor z
 */
void MatrixD::scaleM(std::vector<double> &sm, int smOffset, std::vector<double> &m, int mOffset, double x, double y, double z) {
    for (int i = 0; i < 4; i++) {
        int smi = smOffset + i;
        int mi = mOffset + i;
        sm[smi] = m[mi] * x;
        sm[4 + smi] = m[4 + mi] * y;
        sm[8 + smi] = m[8 + mi] * z;
        sm[12 + smi] = m[12 + mi];
    }
}

/**
 * Scales matrix m in place by sx, sy, and sz.
 *
 * @param m matrix to zoom
 * @param mOffset index into m where the matrix starts
 * @param x zoom factor x
 * @param y zoom factor y
 * @param z zoom factor z
 */
void MatrixD::scaleM(std::vector<double> &m, int mOffset, double x, double y, double z) {
    for (int i = 0; i < 4; i++) {
        int mi = mOffset + i;
        m[mi] *= x;
        m[4 + mi] *= y;
        m[8 + mi] *= z;
    }
}

/**
 * Applies a matrix m to a translation matrix defined by x, y, and z, putting the result in tm.
 * i.e. tm = m * T
 * <p>
 * m and tm must not overlap.
 *
 * @param tm returns the result
 * @param tmOffset index into sm where the result matrix starts
 * @param m source matrix
 * @param mOffset index into m where the source matrix starts
 * @param x translation factor x
 * @param y translation factor y
 * @param z translation factor z
 */
void MatrixD::translateM(std::vector<double> &tm, int tmOffset, std::vector<double> &m, int mOffset, double x, double y, double z) {
    for (int i = 0; i < 12; i++) {
        tm[tmOffset + i] = m[mOffset + i];
    }
    for (int i = 0; i < 4; i++) {
        int tmi = tmOffset + i;
        int mi = mOffset + i;
        tm[12 + tmi] = m[mi] * x + m[4 + mi] * y + m[8 + mi] * z + m[12 + mi];
    }
}

/**
 * Applies a matrix m to a translation matrix defined by x, y, and z in place.
 * i.e. tm = m * T
 *
 * @param m matrix
 * @param mOffset index into m where the matrix starts
 * @param x translation factor x
 * @param y translation factor y
 * @param z translation factor z
 */
void MatrixD::translateM(std::vector<double> &m, int mOffset, double x, double y, double z) {
    for (int i = 0; i < 4; i++) {
        int mi = mOffset + i;
        m[12 + mi] += m[mi] * x + m[4 + mi] * y + m[8 + mi] * z;
    }
}

/**
 * Translates a matrix by by x, y, and z, putting the result in tm.
 * i.e. tm = T * m
 * <p>
 * m and tm must not overlap.
 *
 * @param tm returns the result
 * @param tmOffset index into sm where the result matrix starts
 * @param m source matrix
 * @param mOffset index into m where the source matrix starts
 * @param x translation factor x
 * @param y translation factor y
 * @param z translation factor z
 */
void MatrixD::mTranslated(std::vector<double> &tm, int tmOffset, std::vector<double> &m, int mOffset, double x, double y, double z) {
    for (int i = 0; i <= 12; i += 4) {
        tm[tmOffset + i] = m[mOffset + i] + x * m[mOffset + i + 3];
        tm[tmOffset + i + 1] = m[mOffset + i + 1] + y * m[mOffset + i + 3];
        tm[tmOffset + i + 2] = m[mOffset + i + 2] + z * m[mOffset + i + 3];
        tm[tmOffset + i + 3] = m[mOffset + i + 3];
    }
}

/**
 * Translates a matrix by by x, y, and z in place.
 * i.e. m' = T * m
 *
 * @param m matrix
 * @param mOffset index into m where the matrix starts
 * @param x translation factor x
 * @param y translation factor y
 * @param z translation factor z
 */
void MatrixD::mTranslated(std::vector<double> &m, int mOffset, double x, double y, double z) {
    for (int i = 0; i <= 12; i += 4) {
        m[mOffset + i] += x * m[mOffset + i + 3];
        m[mOffset + i + 1] += y * m[mOffset + i + 3];
        m[mOffset + i + 2] += z * m[mOffset + i + 3];
    }
}

/**
 * Defines a viewing transformation in terms of an eye point, a center of
 * view, and an up vector.
 *
 * @param rm returns the result
 * @param rmOffset index into rm where the result matrix starts
 * @param eyeX eye point X
 * @param eyeY eye point Y
 * @param eyeZ eye point Z
 * @param centerX center of view X
 * @param centerY center of view Y
 * @param centerZ center of view Z
 * @param upX up vector X
 * @param upY up vector Y
 * @param upZ up vector Z
 */
void MatrixD::setLookAtM(std::vector<double> &rm, int rmOffset, double eyeX, double eyeY, double eyeZ, double centerX, double centerY,
                        double centerZ, double upX, double upY, double upZ) {

    // See the OpenGL GLUT documentation for gluLookAt for a description
    // of the algorithm. We implement it in a straightforward way:

    double fx = centerX - eyeX;
    double fy = centerY - eyeY;
    double fz = centerZ - eyeZ;

    // Normalize f
    double rlf = 1.0f / MatrixD::length(fx, fy, fz);
    fx *= rlf;
    fy *= rlf;
    fz *= rlf;

    // compute s = f x up (x means "cross product")
    double sx = fy * upZ - fz * upY;
    double sy = fz * upX - fx * upZ;
    double sz = fx * upY - fy * upX;

    // and normalize s
    double rls = 1.0f / MatrixD::length(sx, sy, sz);
    sx *= rls;
    sy *= rls;
    sz *= rls;

    // compute u = s x f
    double ux = sy * fz - sz * fy;
    double uy = sz * fx - sx * fz;
    double uz = sx * fy - sy * fx;

    rm[rmOffset + 0] = sx;
    rm[rmOffset + 1] = ux;
    rm[rmOffset + 2] = -fx;
    rm[rmOffset + 3] = 0.0f;

    rm[rmOffset + 4] = sy;
    rm[rmOffset + 5] = uy;
    rm[rmOffset + 6] = -fy;
    rm[rmOffset + 7] = 0.0f;

    rm[rmOffset + 8] = sz;
    rm[rmOffset + 9] = uz;
    rm[rmOffset + 10] = -fz;
    rm[rmOffset + 11] = 0.0f;

    rm[rmOffset + 12] = 0.0f;
    rm[rmOffset + 13] = 0.0f;
    rm[rmOffset + 14] = 0.0f;
    rm[rmOffset + 15] = 1.0f;

    MatrixD::translateM(rm, rmOffset, -eyeX, -eyeY, -eyeZ);
}

std::vector<double> MatrixD::sTemp = std::vector<double>(32, 0);

/**
 * Rotates matrix m in place by angle a (in degrees)
 * around the axis (x, y, z).
 *
 * @param m source matrix
 * @param mOffset index into m where the matrix starts
 * @param a angle to rotate in degrees
 * @param x X axis component
 * @param y Y axis component
 * @param z Z axis component
 */
void MatrixD::rotateM(std::vector<double> &m, int mOffset, double a, double x, double y, double z) {
    setRotateM(sTemp, 0, a, x, y, z);
    multiplyMM(sTemp, 16, m, mOffset, sTemp, 0);
    for (int i = 0; i < 16; i++) {
        m[mOffset + i] = sTemp[16 + i];
    }
}

/**
 * Creates a matrix for rotation by angle a (in degrees)
 * around the axis (x, y, z).
 * <p>
 * An optimized path will be used for rotation about a major axis
 * (e.g. x=1.0f y=0.0f z=0.0f).
 *
 * @param rm returns the result
 * @param rmOffset index into rm where the result matrix starts
 * @param a angle to rotate in degrees
 * @param x X axis component
 * @param y Y axis component
 * @param z Z axis component
 */
void MatrixD::setRotateM(std::vector<double> &rm, int rmOffset, double a, double x, double y, double z) {
    rm[rmOffset + 3] = 0;
    rm[rmOffset + 7] = 0;
    rm[rmOffset + 11] = 0;
    rm[rmOffset + 12] = 0;
    rm[rmOffset + 13] = 0;
    rm[rmOffset + 14] = 0;
    rm[rmOffset + 15] = 1;
    a *= (double)(M_PI / 180.0f);
    double s = (double)sin(a);
    double c = (double)cos(a);
    if (1.0f == x && 0.0f == y && 0.0f == z) {
        rm[rmOffset + 5] = c;
        rm[rmOffset + 10] = c;
        rm[rmOffset + 6] = s;
        rm[rmOffset + 9] = -s;
        rm[rmOffset + 1] = 0;
        rm[rmOffset + 2] = 0;
        rm[rmOffset + 4] = 0;
        rm[rmOffset + 8] = 0;
        rm[rmOffset + 0] = 1;
    } else if (0.0f == x && 1.0f == y && 0.0f == z) {
        rm[rmOffset + 0] = c;
        rm[rmOffset + 10] = c;
        rm[rmOffset + 8] = s;
        rm[rmOffset + 2] = -s;
        rm[rmOffset + 1] = 0;
        rm[rmOffset + 4] = 0;
        rm[rmOffset + 6] = 0;
        rm[rmOffset + 9] = 0;
        rm[rmOffset + 5] = 1;
    } else if (0.0f == x && 0.0f == y && 1.0f == z) {
        rm[rmOffset + 0] = c;
        rm[rmOffset + 5] = c;
        rm[rmOffset + 1] = s;
        rm[rmOffset + 4] = -s;
        rm[rmOffset + 2] = 0;
        rm[rmOffset + 6] = 0;
        rm[rmOffset + 8] = 0;
        rm[rmOffset + 9] = 0;
        rm[rmOffset + 10] = 1;
    } else {
        double len = length(x, y, z);
        if (1.0f != len) {
            double recipLen = 1.0f / len;
            x *= recipLen;
            y *= recipLen;
            z *= recipLen;
        }
        double nc = 1.0f - c;
        double xy = x * y;
        double yz = y * z;
        double zx = z * x;
        double xs = x * s;
        double ys = y * s;
        double zs = z * s;
        rm[rmOffset + 0] = x * x * nc + c;
        rm[rmOffset + 4] = xy * nc - zs;
        rm[rmOffset + 8] = zx * nc + ys;
        rm[rmOffset + 1] = xy * nc + zs;
        rm[rmOffset + 5] = y * y * nc + c;
        rm[rmOffset + 9] = yz * nc - xs;
        rm[rmOffset + 2] = zx * nc - ys;
        rm[rmOffset + 6] = yz * nc + xs;
        rm[rmOffset + 10] = z * z * nc + c;
    }
}

#define I(_i, _j) ((_j) + 4 * (_i))

void MatrixD::multiplyMM(std::vector<double> &r, int resultOffset, std::vector<double> &lhs, int lhsOffset, std::vector<double> &rhs,
                        int rhsOffset) {
    multiplyMMC(r, resultOffset, lhs, lhsOffset, rhs, rhsOffset);
}

void MatrixD::multiplyMMC(std::vector<double> &r, int resultOffset, const std::vector<double> &lhs, int lhsOffset,
                         const std::vector<double> &rhs, int rhsOffset) {
    for (int i = 0; i < 4; i++) {
        const double rhs_i0 = rhs[rhsOffset + I(i, 0)];
        double ri0 = lhs[lhsOffset + I(0, 0)] * rhs_i0;
        double ri1 = lhs[lhsOffset + I(0, 1)] * rhs_i0;
        double ri2 = lhs[lhsOffset + I(0, 2)] * rhs_i0;
        double ri3 = lhs[lhsOffset + I(0, 3)] * rhs_i0;
        for (int j = 1; j < 4; j++) {
            const double rhs_ij = rhs[rhsOffset + I(i, j)];
            ri0 += lhs[lhsOffset + I(j, 0)] * rhs_ij;
            ri1 += lhs[lhsOffset + I(j, 1)] * rhs_ij;
            ri2 += lhs[lhsOffset + I(j, 2)] * rhs_ij;
            ri3 += lhs[lhsOffset + I(j, 3)] * rhs_ij;
        }
        r[resultOffset + I(i, 0)] = ri0;
        r[resultOffset + I(i, 1)] = ri1;
        r[resultOffset + I(i, 2)] = ri2;
        r[resultOffset + I(i, 3)] = ri3;
    }
}

Vec4D MatrixD::multiply(const std::vector<double> &M, const Vec4D &x) {
    // calculates Mx and returns result
    Vec4D result = Vec4D(0.0, 0.0, 0.0, 0.0);
    MatrixD::multiply(M, x, result);
    return result;
}

void MatrixD::multiply(const std::vector<double> &M, const Vec4D &x, Vec4D &result) {
    result.x = M[0] * x.x + M[4] * x.y + M[8] * x.z + M[12] * x.w;
    result.y = M[1] * x.x + M[5] * x.y + M[9] * x.z + M[13] * x.w;
    result.z = M[2] * x.x + M[6] * x.y + M[10] * x.z + M[14] * x.w;
    result.w = M[3] * x.x + M[7] * x.y + M[11] * x.z + M[15] * x.w;
}

std::string MatrixD::toMatrixString(const std::vector<double> &M) {
    std::stringstream ss;
    ss << "[ " << M[0] << ", " << M[4] << ", " << M[8] << ", " << M[12] << "; "
    << M[1] << ", " << M[5] << ", " << M[9] << ", " << M[13] << "; "
    << M[2] << ", " << M[6] << ", " << M[10] << ", " << M[14] << "; "
    << M[3] << ", " << M[7] << ", " << M[11] << ", " << M[15] << " ]";
    return ss.str();
}

std::string MatrixD::toVectorString(const std::vector<double> &v) {
    std::stringstream ss;
    ss << "[ ";
    for (int i = 0; i < v.size(); i++) {
        ss << v[i];
        if (i < v.size() - 1) {
            ss << "; ";
        }
    }
    ss << " ]";
    return ss.str();
}
