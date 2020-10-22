/*
 * Copyright (c) 2015, M.Naruoka (fenrir)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of the naruoka.org nor the names of its contributors
 *   may be used to endorse or promote products derived from this software
 *   without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __INTEGRAL_H
#define __INTEGRAL_H

/** @file
 * @brief �ϕ��@���L�q�����t�@�C���ł��B
 * 
 * �ϕ��@���L�q�����t�@�C���ł��B
 * ���݂̓I�C���[�@�ARunge-Kutta�@(2���A4��)��3��ނ̐ϕ��@���T�|�[�g���Ă��܂��B
 *  
 */

/**
 * Runge-Kutta�@(4��)�ɂ��ϕ�
 * 
 * @param f ����������\f$ f(x, y) \f$
 * @param x ��ԗ�
 * @param y �ړI��
 * @param h ��ԗʂ̍���
 * @return �w���ԗʍ������o�߂����ۂ̖ړI��
 */
template <class Function, class V1, class V2>
V2 nextByRK4(const Function &f, const V1 &x, const V2 &y, const V1 &h){
  V2 k1(f(x, y) * h);
  V2 k2(f(x + h/2, y + k1/2) * h);
  V2 k3(f(x + h/2, y + k2/2) * h);
  V2 k4(f(x + h, y + k3) * h);
  return y + (k1 + k2*2 + k3*2 + k4)/6;
}

/**
 * Runge-Kutta�@(2��)�ɂ��ϕ�
 * 
 * @param f ����������\f$ f(x, y) \f$
 * @param x ��ԗ�
 * @param y �ړI��
 * @param h ��ԗʂ̍���
 * @return �w���ԗʍ������o�߂����ۂ̖ړI��
 */
template <class Function, class V1, class V2>
V2 nextByRK2(const Function &f, const V1 &x, const V2 &y, const V1 &h){
  V2 k1(f(x, y) * h);
  V2 k2(f(x + h, y + k1) * h);
  return y + (k1 + k2)/2;
}

/**
 * Euler�@(1��)�ɂ��ϕ�
 * 
 * @param f ����������\f$ f(x, y) \f$
 * @param x ��ԗ�
 * @param y �ړI��
 * @param h ��ԗʂ̍���
 * @return �w���ԗʍ������o�߂����ۂ̖ړI��
 */
template <class Function, class V1, class V2>
V2 nextByEuler(const Function &f, const V1 &x, const V2 &y, const V1 &h){
  return y + f(x, y) * h;
}

#endif /* __INTEGRAL_H */
