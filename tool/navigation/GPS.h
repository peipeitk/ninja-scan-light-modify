/*
 * Copyright (c) 2016, M.Naruoka (fenrir)
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

#ifndef __GPS_H__
#define __GPS_H__

/** @file
 * @brief GPS ICD definitions including C/A code, time, ephemeris, ...
 */

#include <vector>
#include <iterator>
#include <map>
#include <bitset>

#include <ctime>
#define _USE_MATH_DEFINES
#include <cmath>
#include <climits>
#include <cstdlib>

#include "WGS84.h"

#include "coordinate.h"

#ifdef pow2
#define POW2_ALREADY_DEFINED
#else
#define pow2(x) ((x)*(x))
#endif
#ifdef pow3
#define POW3_ALREADY_DEFINED
#else
#define pow3(x) ((x)*(x)*(x))
#endif

template <class FloatT = double>
class GPS_Signal {
  public:
    typedef FloatT float_t;

    class PRN {
      public:
        typedef std::bitset<10> content_t;
      protected:
        content_t content;
      public:
        void reset(){content.set();}
        PRN() : content() {reset();}
        PRN(const unsigned long &init) : content(init) {}
        ~PRN(){}
        friend std::ostream &operator<<(std::ostream &out, const PRN &prn){
          out << const_cast<PRN &>(prn).content;
          return out;
        }
    };
    
    class G1 : public PRN {
      public:
        G1() : PRN() {}
        ~G1(){}
        bool get() const {return PRN::content[9];}
        void next(){
          bool tmp(PRN::content[2] ^ PRN::content[9]);
          PRN::content <<= 1;
          PRN::content[0] = tmp;
        }
    };
    
    class G2 : public PRN {
      protected:
        const int _selector1, _selector2;
      public:
        G2(int selector1, int selector2) : PRN(), _selector1(selector1), _selector2(selector2) {}
        ~G2(){}
        bool get() const {return PRN::content[_selector1] ^ PRN::content[_selector2];}
        void next(){
          bool tmp(PRN::content[1]
                     ^ PRN::content[2]
                     ^ PRN::content[5]
                     ^ PRN::content[7]
                     ^ PRN::content[8]
                     ^ PRN::content[9]);
          PRN::content <<= 1;
          PRN::content[0] = tmp;
        }
        static G2 get_G2(const int &prn){
          switch(prn){
            case  1: return G2(1, 5);
            case  2: return G2(2, 6);
            case  3: return G2(3, 7);
            case  4: return G2(4, 8);
            case  5: return G2(0, 8);
            case  6: return G2(1, 9);
            case  7: return G2(0, 7);
            case  8: return G2(1, 8);
            case  9: return G2(2, 9);
            case 10: return G2(1, 2);
            case 11: return G2(2, 3);
            case 12: return G2(4, 5);
            case 13: return G2(5, 6);
            case 14: return G2(6, 7);
            case 15: return G2(7, 8);
            case 16: return G2(8, 9);
            case 17: return G2(0, 3);
            case 18: return G2(1, 4);
            case 19: return G2(2, 5);
            case 20: return G2(3, 6);
            case 21: return G2(4, 7);
            case 22: return G2(5, 8);
            case 23: return G2(0, 2);
            case 24: return G2(3, 5);
            case 25: return G2(4, 6);
            case 26: return G2(5, 7);
            case 27: return G2(6, 8);
            case 28: return G2(7, 9);
            case 29: return G2(0, 5);
            case 30: return G2(1, 6);
            case 31: return G2(2, 7);
            case 32: return G2(3, 8);
            case 33: return G2(4, 9);
            case 34: return G2(3, 9);
            case 35: return G2(0, 6);
            case 36: return G2(1, 7);
            default:  return G2(3, 9);
          }
        }
    };

    class CA_Code {
      public:
        typedef FloatT float_t;
        static const float_t FREQENCY;
        static const float_t length_1chip() {
          static const float_t res(1. / FREQENCY);
          return res;
        }
      protected:
        G1 g1;
        G2 g2;
      public:
        CA_Code(const int &prn) : g1(), g2(G2::get_G2(prn)){}
        ~CA_Code(){}
        bool get() const {return g1.get() ^ g2.get();}
        int get_multi() const {return get() ? 1 : -1;}
        void next(){
          g1.next();
          g2.next();
        }
    };
};

template <class FloatT>
const typename GPS_Signal<FloatT>::float_t GPS_Signal<FloatT>::CA_Code::FREQENCY = 1.023E6;

template <class FloatT = double>
struct GPS_Time {
  typedef FloatT float_t;
  static const unsigned int seconds_day = 60U * 60 * 24;
  static const unsigned int seconds_week = (60U * 60 * 24) * 7;
  
  static const int days_of_month[];
  
  /**
   * Check whether leap year
   *
   * @param year
   * @return true when leap year, otherwise false
   */
  static inline  bool is_leap_year(const int &year) {
    return (year % 400 == 0) || ((year % 4 == 0) && (year % 100 != 0));
  }

  struct leap_year_prop_res_t {
    int extra_days; ///< extra leap years since 1980
    bool is_leap_year; ///< true when leap year
  };

  /**
   * Check leap year property
   * The return values are;
   * 1) extra_days equals to years which can be divided by 4, but are not leap years
   * since 1980 (the first GPS year), and before (and except for) this_year.
   * 2) is_leap_year equals to whether this_year is leap year or not.
   *
   * @param this_year check target
   * @param skip_init_leap_year_check whether skip initial check whether this_year is leap year or not.
   * @return leap year property
   */
  static leap_year_prop_res_t leap_year_prop(const int &this_year, const bool &skip_init_leap_year_check = false){
    leap_year_prop_res_t res = {0, skip_init_leap_year_check || (this_year % 4 == 0)};
    do{ // check leap year
      std::div_t y_400(std::div(this_year, 400));
      if((y_400.quot -= 5) < 0){break;} // year < 2000
      res.extra_days += (y_400.quot * 3); // no leap year; [2100, 2200, 2300], [2500, ...
      if(y_400.rem == 0){break;}
      std::div_t y_100(std::div(y_400.rem, 100));
      res.extra_days += y_100.quot;
      if(y_100.rem == 0){ // when this_year is just 2100, 2200, 2300, or 2500, ...
        res.extra_days--;
        res.is_leap_year = false;
      }
    }while(false);
    return res;
  }

  int week;
  float_t seconds;
  
  GPS_Time() {}
  GPS_Time(const GPS_Time &t)
      : week(t.week), seconds(t.seconds) {}
  GPS_Time(const int &_week, const float_t &_seconds)
      : week(_week), seconds(_seconds) {}
  GPS_Time &canonicalize(){
    int quot(std::floor(seconds / seconds_week));
    week += quot;
    seconds -= (seconds_week * quot);
    return *this;
  }
  GPS_Time(const struct tm &t, const float_t &leap_seconds = 0) {
    int days(-6);
    int y(t.tm_year + 1900); // tm_year is year minus 1900
    bool leap_year;
    {
      leap_year_prop_res_t prop(leap_year_prop(y));
      days -= prop.extra_days;
      leap_year = prop.is_leap_year;
    }

    y -= 1980; // base is 1980/1/6
    days += y * 365 + ((y + 3) / 4);
    for(int i(0); i < t.tm_mon; i++){
      days += days_of_month[i];
      if((i == 1) && leap_year){days++;}
    }
    days += t.tm_mday;
    
    std::div_t week_day(std::div(days, 7));
    week = week_day.quot;
    seconds = leap_seconds + week_day.rem * seconds_day
        + t.tm_hour * 60 * 60
        + t.tm_min * 60
        + t.tm_sec;
    canonicalize();
  }
  static GPS_Time now(const float_t &leap_seconds = 0) {
    time_t timer;
    struct tm t;
    
    time(&timer); // Current serial time, then convert the time to struct
#if defined(_MSC_VER)
    gmtime_s(&t, &timer);
#elif defined(__GNUC__)
    gmtime_r(&timer, &t);
#else
    t = *gmtime(&timer);
#endif
    
    return GPS_Time(t, leap_seconds);
  }
  float_t serialize() const {
    return seconds + (float_t)seconds_week * week;
  }
  
  GPS_Time &operator+=(const float_t &sec){
    seconds += sec;
    canonicalize();
    return *this;
  }
  GPS_Time &operator-=(const float_t &sec){
    return operator+=(-sec);
  }
  GPS_Time operator+(const float_t &sec) const {
    GPS_Time t(*this);
    return (t += sec);
  }
  GPS_Time operator-(const float_t &sec) const {
    return operator+(-sec);
  }
  /**
   * Get interval in unit of second
   */
  float_t operator-(const GPS_Time &t) const {
    float_t res(seconds - t.seconds);
    res += ((float_t)week - t.week) * seconds_week;
    return res;
  }
  friend float_t operator+(float_t v, const GPS_Time &t){
    return v + (((float_t)t.week * seconds_week) + t.seconds);
  }
  friend float_t operator-(float_t v, const GPS_Time &t){
    return v - (((float_t)t.week * seconds_week) + t.seconds);
  }
  bool operator<(const GPS_Time &t) const {
    return ((week < t.week) ? true : ((week > t.week) ? false : (seconds < t.seconds)));
  }
  bool operator>(const GPS_Time &t) const {
    return t.operator<(*this);
  }
  bool operator==(const GPS_Time &t) const {
    return (week == t.week) && (seconds == t.seconds);
  }
  bool operator!=(const GPS_Time &t) const {
    return !(operator==(t));
  }
  bool operator<=(const GPS_Time &t) const {
    return ((week < t.week) ? true : ((week > t.week) ? false : (seconds <= t.seconds)));
  }
  bool operator>=(const GPS_Time &t) const {
    return t.operator<=(*this);
  }
  
  struct tm c_tm(const float_t &leap_seconds = 0) const {
    struct tm t;
    
    GPS_Time mod_t((*this) + leap_seconds);
    
    std::div_t min_sec(std::div((int)mod_t.seconds, 60));
    t.tm_sec = min_sec.rem;
    std::div_t hr_min(std::div(min_sec.quot, 60));
    t.tm_min = hr_min.rem;
    std::div_t day_hr(std::div(hr_min.quot, 24));
    t.tm_hour = day_hr.rem;
    t.tm_wday = t.tm_mday = day_hr.quot;

    t.tm_mday += 6 + (mod_t.week * 7);
    std::div_t days_4year(std::div(t.tm_mday, 366 + 365 * 3)); // standard day of years
    t.tm_mday = days_4year.rem;
    int y(days_4year.quot * 4 + 1980);
    bool leap_year;
    {
      leap_year_prop_res_t prop(leap_year_prop(y, true));
      t.tm_mday += prop.extra_days;
      leap_year = prop.is_leap_year;
    }

    // process remaining 4 years
    int doy[] = {
      leap_year ? 366 : 365,
      365, 365, 365
    };
    for(int i(0); i < sizeof(doy) / sizeof(doy[0]); ++i){
      if(t.tm_mday <= doy[i]){break;}
      t.tm_mday -= doy[i];
      y++;
    }

    // process current year
    leap_year = is_leap_year(y);
    t.tm_yday = t.tm_mday;
    t.tm_year = y - 1900; // tm_year is year minus 1900.
    for(t.tm_mon = 0; 
        t.tm_mday > days_of_month[t.tm_mon];
        (t.tm_mon)++){
      if((t.tm_mon == 1) && leap_year){
        if(t.tm_mday == 29){break;}
        else{t.tm_mday--;}
      }
      t.tm_mday -= days_of_month[t.tm_mon];
    }
    t.tm_isdst = 0;
    
    return t;
  }

  float_t year(const float_t &leap_seconds = 0) const {
    float_t days((seconds + leap_seconds) / seconds_day + (week * 7) + (6 - 1)); // days from 1980/1/1, whose 00:00:00 is just 0
    float_t year4;
    days = std::modf(days / (366 + 365 * 3), &year4) * (366 + 365 * 3);
    int year(1980 + (int)year4 * 4);
    bool leap_year;
    {
      leap_year_prop_res_t prop(leap_year_prop(year, true));
      days += prop.extra_days;
      leap_year = prop.is_leap_year;
    }

    // process remaining 4 years
    int doy_i(0), doy[] = {
      leap_year ? 366 : 365,
      365, 365, 365,
      is_leap_year(year + 4) ? 366 : 365,
    };
    for(; doy_i < sizeof(doy) / sizeof(doy[0]); ++doy_i){
      if(days <= doy[doy_i]){break;}
      days -= doy[doy_i];
      year++;
    }

    return days / doy[doy_i] + year;
  }
  
  /**
   * When t >= self positive value will be returned,
   * otherwise(t  < self), negative.
   */
  float_t interval(const unsigned int &t_week,
      const float_t &t_seconds) const {
    return t_seconds - seconds
        + ((float_t)t_week - week) * seconds_week;
  }
  float_t interval(const GPS_Time &t) const {
    return interval(t.week, t.seconds);
  }

  friend std::ostream &operator<<(std::ostream &out, const GPS_Time &t){
    out << t.week << " week " << t.seconds << " sec.";
    return out;
  }
};

template <class FloatT>
const int GPS_Time<FloatT>::days_of_month[] = {
      31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };

template <class FloatT = double>
class GPS_SpaceNode {
  public:
    typedef FloatT float_t;
    static const float_t light_speed;
    static const float_t L1_Frequency;
    static const float_t &L1_WaveLength() {
      static const float_t res(light_speed / L1_Frequency);
      return res;
    }
    static const float_t SC2RAD;
    
    static const float_t L2_Frequency;
    static const float_t &L2_WaveLength() {
      static const float_t res(light_speed / L2_Frequency);
      return res;
    }
    static const float_t gamma_L1_L2;

  protected:
    static float_t rad2sc(const float_t &rad) {return rad / M_PI;}
    static float_t sc2rad(const float_t &sc)  {return sc * M_PI;}
    
  public:
    typedef GPS_SpaceNode<float_t> self_t;
    typedef GPS_Time<float_t> gps_time_t;
    typedef System_XYZ<float_t, WGS84> xyz_t;
    typedef System_LLH<float_t, WGS84> llh_t;
    typedef System_ENU<float_t, WGS84> enu_t;

    typedef unsigned char u8_t;
    typedef signed char s8_t;
    typedef unsigned short u16_t;
    typedef signed short s16_t;
    typedef unsigned int u32_t;
    typedef signed int s32_t;
    
    typedef int int_t;
    typedef unsigned int uint_t;

    struct DataParser {
      template <
          class OutputT, class InputT,
          int EffectiveBits_in_InputT = sizeof(InputT) * CHAR_BIT,
          int PaddingBits_in_InputT_MSB = (int)sizeof(InputT) * CHAR_BIT - EffectiveBits_in_InputT,
          bool output_is_smaller_than_input
            = (EffectiveBits_in_InputT >= ((int)sizeof(OutputT) * CHAR_BIT))>
      struct bits2num_t {
        static OutputT run(const InputT *buf, const uint_t &index){
          // ex.1) I_8 0 1 2 3 4 5 6 7 | 8 9 0 1 2 3 4 5
          //       O_8         0*1*2*3* *4*5*6*7
          // ex.2) I_16 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 | 6 7 8 9 0 1 2 3
          //       O_8        0*1*2*3*4*5*6*7
          // ex.3) I_16 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 | 6 7 8 9 0 1 2 3
          //       O_8                          0*1*2*3* *4*5*6*7
          static const int
              output_bits(sizeof(OutputT) * CHAR_BIT),
              input_bits(sizeof(InputT) * CHAR_BIT);
          static const int padding_bits_MSB_abs(
              PaddingBits_in_InputT_MSB * (PaddingBits_in_InputT_MSB >= 0 ? 1 : -1));
          std::div_t aligned(std::div(index, EffectiveBits_in_InputT));
          if(PaddingBits_in_InputT_MSB >= 0){
            OutputT res(
                (buf[aligned.quot] << (aligned.rem + padding_bits_MSB_abs))
                  >> (input_bits - output_bits));
            if(aligned.rem > (EffectiveBits_in_InputT - output_bits)){
              // in case of overrun; ex.1 and ex.3
              res |= (OutputT)(
                  (buf[++aligned.quot] << padding_bits_MSB_abs)
                    // left shift to remove padding
                    >> (EffectiveBits_in_InputT
                       + (input_bits - output_bits) - aligned.rem));
                    // right shift to fill remaining;
                    // shift = input - [require_bits = output - (effective - rem)]
            }
            return res;
          }else{
            // rare case: negative MSB padding
            int left_shift(aligned.rem + PaddingBits_in_InputT_MSB);
            OutputT res(
                (buf[aligned.quot] << (left_shift >= 0 ? left_shift : 0))
                  >> (input_bits - output_bits + (left_shift >= 0 ? 0 : -left_shift)));
            if(aligned.rem > (EffectiveBits_in_InputT - output_bits)){
              res |= (OutputT)(
                  buf[++aligned.quot]
                    >> (EffectiveBits_in_InputT + padding_bits_MSB_abs
                       + (input_bits - output_bits) - aligned.rem));
            }
            return res;
          }
        }
      };
      template <class OutputT, class InputT,
          int EffectiveBits_in_InputT, int PaddingBits_in_InputT_MSB>
      struct bits2num_t<OutputT, InputT,
          EffectiveBits_in_InputT, PaddingBits_in_InputT_MSB, false> {
        static OutputT run(const InputT *buf, const uint_t &index){
          // When sizeof(OutputT) > sizeof(InputT)
          // ex.4) I_8  0 1 2 3 4 5 6 7 | 8 9 0 1 2 3 4 5 | 6 7 8 9 0 1 2 3
          //       O_16         0*1*2*3* *4*5*6*7*8*9*0*1* *2*3*4*5
          static const int
              output_bits(sizeof(OutputT) * CHAR_BIT),
              input_bits(sizeof(InputT) * CHAR_BIT);
          static const int padding_bits_LSB(
              input_bits - EffectiveBits_in_InputT - PaddingBits_in_InputT_MSB);
          static const int padding_bits_LSB_abs(
              padding_bits_LSB >= 0 ? padding_bits_LSB : -padding_bits_LSB);
          static const int effective_mask_shift(
              (PaddingBits_in_InputT_MSB <= 0) ? 0 : (PaddingBits_in_InputT_MSB - 1));
          static const InputT effective_mask((PaddingBits_in_InputT_MSB <= 0)
              ? (~(InputT)0)
              : (((((InputT)1) << (input_bits - 1)) - 1) >> effective_mask_shift));
          static const int shift_after_mask(
              (PaddingBits_in_InputT_MSB < 0) ? -PaddingBits_in_InputT_MSB : 0);
          std::div_t aligned(std::div(index, EffectiveBits_in_InputT));
          OutputT res(0);
          for(int i(output_bits / EffectiveBits_in_InputT); i > 0; --i, ++aligned.quot){
            res <<= EffectiveBits_in_InputT;
            res |= ((padding_bits_LSB >= 0)
                ? (((buf[aligned.quot] & effective_mask) >> shift_after_mask) >> padding_bits_LSB_abs)
                : (((buf[aligned.quot] & effective_mask) >> shift_after_mask) << padding_bits_LSB_abs));
          }
          int last_shift(aligned.rem + (output_bits % EffectiveBits_in_InputT));
          if(last_shift > 0){
            res <<= last_shift;
            res |= (((buf[aligned.quot] & effective_mask) >> shift_after_mask)
                >> (EffectiveBits_in_InputT + padding_bits_LSB - last_shift));
          }
          return res;
        }
      };

      template <class OutputT, class InputT>
      static OutputT bits2num(const InputT *buf, const uint_t &index){
        return bits2num_t<OutputT, InputT>::run(buf, index);
      }
      template <class OutputT, class InputT>
      static OutputT bits2num(const InputT *buf, const uint_t &index, const uint_t &length){
        return (bits2num<OutputT, InputT>(buf, index) >> ((sizeof(OutputT) * CHAR_BIT) - length));
      }

      template <class OutputT,
          int EffectiveBits_in_InputT, int PaddingBits_in_InputT_MSB,
          class InputT>
      static OutputT bits2num(const InputT *buf, const uint_t &index){
        return bits2num_t<OutputT, InputT,
            EffectiveBits_in_InputT, PaddingBits_in_InputT_MSB>::run(buf, index);
      }
      template <class OutputT,
          int EffectiveBits_in_InputT, int PaddingBits_in_InputT_MSB,
          class InputT>
      static OutputT bits2num(const InputT *buf, const uint_t &index, const uint_t &length){
        return (bits2num<OutputT,
              EffectiveBits_in_InputT, PaddingBits_in_InputT_MSB,
              InputT>(buf, index)
            >> ((sizeof(OutputT) * CHAR_BIT) - length));
      }
    };

    template <class InputT,
        int EffectiveBits = sizeof(InputT) * CHAR_BIT,
        int PaddingBits_MSB = (int)sizeof(InputT) * CHAR_BIT - EffectiveBits>
    struct BroadcastedMessage : public DataParser {
#define convert_u(bits, offset_bits, length, name) \
static u ## bits ## _t name(const InputT *buf){ \
  return \
      DataParser::template bits2num<u ## bits ## _t, EffectiveBits, PaddingBits_MSB>( \
        buf, offset_bits, length); \
}
#define convert_s(bits, offset_bits, length, name) \
static s ## bits ## _t name(const InputT *buf){ \
  return ((s ## bits ## _t) \
      DataParser::template bits2num<u ## bits ## _t, EffectiveBits, PaddingBits_MSB>( \
        buf, offset_bits)) \
      >> (bits - length); \
}
#define convert_u_2(bits, offset_bits1, length1, offset_bits2, length2, name) \
static u ## bits ## _t name(const InputT *buf){ \
  return \
      (DataParser::template bits2num<u ## bits ## _t, EffectiveBits, PaddingBits_MSB>( \
        buf, offset_bits1, length1) << length2) \
      | DataParser::template bits2num<u ## bits ## _t, EffectiveBits, PaddingBits_MSB>( \
          buf, offset_bits2, length2); \
}
#define convert_s_2(bits, offset_bits1, length1, offset_bits2, length2, name) \
static s ## bits ## _t name(const InputT *buf){ \
  return ((s ## bits ## _t) \
      ((DataParser::template bits2num<u ## bits ## _t, EffectiveBits, PaddingBits_MSB>( \
          buf, offset_bits1, length1) << (bits - length1)) \
        | (DataParser::template bits2num<u ## bits ## _t, EffectiveBits, PaddingBits_MSB>( \
            buf, offset_bits2, length2) << (bits - length1 - length2)))) \
      >> (bits - length1 - length2); \
}
      convert_u( 8,  0,  8, preamble);
      convert_u(32, 30, 24, how);
      convert_u( 8, 49,  3, subframe_id);

      struct SubFrame1 {
        convert_u(16, 60, 10, WN);
        convert_u( 8, 72,  4, URA);
        convert_u( 8, 76,  6, SV_health);
        convert_u_2(16, 82,  2, 210,  8, iodc);
        convert_s( 8, 196,  8, t_GD);
        convert_u(16, 218, 16, t_oc);
        convert_s( 8, 240,  8, a_f2);
        convert_s(16, 248, 16, a_f1);
        convert_s(32, 270, 22, a_f0);
      };

      struct SubFrame2 {
        convert_u( 8, 60,  8, iode);
        convert_s(16, 68, 16, c_rs);
        convert_s(16, 90, 16, delta_n);
        convert_s_2(32, 106,  8, 120, 24, M0);
        convert_s(16, 150, 16, c_uc);
        convert_u_2(32, 166,  8, 180, 24, e);
        convert_s(16, 210, 16, c_us);
        convert_u_2(32, 226,  8, 240, 24, sqrt_A);
        convert_u(16, 270, 16, t_oe);
        convert_u( 8, 286,  1, fit);
      };

      struct SubFrame3 {
        convert_s(16, 60, 16, c_ic);
        convert_s_2(32,  76,  8,  90, 24, Omega0);
        convert_s(16, 120, 16, c_is);
        convert_s_2(32, 136,  8, 150, 24, i0);
        convert_s(16, 180, 16, c_rc);
        convert_s_2(32, 196,  8, 210, 24, omega);
        convert_s(32, 240, 24, dot_Omega0);
        convert_u( 8, 270,  8, iode);
        convert_s(16, 278, 14, dot_i0);
      };

      convert_u( 8,  62,  6, sv_page_id);

      struct SubFrame4_5_Alnamac {
        convert_u(16,  68, 16, e);
        convert_u( 8,  90,  8, t_oa);
        convert_s(16,  98, 16, delta_i);
        convert_s(16, 120, 16, dot_Omega0);
        convert_u( 8, 128,  8, SV_health);
        convert_u(32, 150, 24, sqrt_A);
        convert_s(32, 180, 24, Omega0);
        convert_s(32, 210, 24, omega);
        convert_s(32, 240, 24, M0);
        convert_s_2(16, 270,  8, 289,  3, a_f0);
        convert_s(16, 278, 11, a_f1);
      };

      struct SubFrame4_Page18 {
        convert_s( 8,  68,  8, alpha0);
        convert_s( 8,  76,  8, alpha1);
        convert_s( 8,  90,  8, alpha2);
        convert_s( 8,  98,  8, alpha3);
        convert_s( 8, 106,  8, beta0);
        convert_s( 8, 120,  8, beta1);
        convert_s( 8, 128,  8, beta2);
        convert_s( 8, 136,  8, beta3);
        convert_s(32, 150, 24, A1);
        convert_s_2(32, 180, 24, 210,  8, A0);
        convert_u( 8, 218,  8, t_ot);
        convert_s( 8, 240,  8, delta_t_LS);
        convert_u( 8, 226,  8, WN_t);
        convert_u( 8, 248,  8, WN_LSF);
        convert_u( 8, 256,  8, DN);
        convert_s( 8, 270,  8, delta_t_LSF);
      };
#undef convert_s_2
#undef convert_u_2
#undef convert_s
#undef convert_u
    };

    /**
     * GPS Ionospheric correction and UTC parameters
     * 
     */
    struct Ionospheric_UTC_Parameters {
      float_t alpha[4];    ///< Ionospheric parameters[0-3] (s, s/sc, s/sc^2, s/sc^3)
      float_t beta[4];     ///< Ionospheric parameters[0-3] (s, s/sc, s/sc^2, s/sc^3)
      float_t A1;          ///< UTC parameter (s/s)
      float_t A0;          ///< UTC parameter (s)
      uint_t t_ot;         ///< Epoch time (UTC) (s)
      uint_t WN_t;         ///< Epoch time (UTC) (weeks)
      int_t delta_t_LS;    ///< Current leap seconds (s)
      uint_t WN_LSF;       ///< Last leap second update week (weeks)
      uint_t DN;           ///< Last leap second update day (days)
      int_t delta_t_LSF;   ///< Updated leap seconds (s)

      struct raw_t {
        s8_t  alpha0;       ///< Ionospheric parameter (-30, s)
        s8_t  alpha1;       ///< Ionospheric parameter (-27, s/sc)
        s8_t  alpha2;       ///< Ionospheric parameter (-24, s/sc^2)
        s8_t  alpha3;       ///< Ionospheric parameter (-24, s/sc^3)
        s8_t  beta0;        ///< Ionospheric parameter (11, s)
        s8_t  beta1;        ///< Ionospheric parameter (14, s/sc)
        s8_t  beta2;        ///< Ionospheric parameter (16, s/sc^2)
        s8_t  beta3;        ///< Ionospheric parameter (16, s/sc^3)
        s32_t A1;           ///< UTC parameter (-50, s/s)
        s32_t A0;           ///< UTC parameter (-30, s)
        u8_t  t_ot;         ///< Epoch time (UTC) (12, s)
        u8_t  WN_t;         ///< Epoch time (UTC) (weeks, truncated)
        s8_t  delta_t_LS;   ///< Current leap seconds (s)
        u8_t  WN_LSF;       ///< Last leap second update week (weeks, truncated)
        u8_t  DN;           ///< Last leap second update day (days)
        s8_t  delta_t_LSF;  ///< Updated leap seconds (s)

#define fetch_item(name) name = BroadcastedMessage< \
   InputT, (int)sizeof(InputT) * CHAR_BIT - PaddingBits_MSB - PaddingBits_LSB, PaddingBits_MSB> \
   :: SubFrame4_Page18 :: name (src)
        template <int PaddingBits_MSB, int PaddingBits_LSB, class InputT>
        void update(const InputT *src){
          fetch_item(alpha0); fetch_item(alpha1); fetch_item(alpha2); fetch_item(alpha3);
          fetch_item(beta0);  fetch_item(beta1);  fetch_item(beta2);  fetch_item(beta3);
          fetch_item(A1);     fetch_item(A0);
          fetch_item(WN_t);   fetch_item(WN_LSF);
          fetch_item(t_ot);   fetch_item(delta_t_LS);   fetch_item(delta_t_LSF);
          fetch_item(DN);
        }
#undef fetch_item
        
        enum {
          SF_alpha0,
          SF_alpha1,
          SF_alpha2,
          SF_alpha3,
          SF_beta0,
          SF_beta1,
          SF_beta2,
          SF_beta3,
          SF_A1,
          SF_A0,

          SF_NUM,
        };
        static const float_t sf[SF_NUM];

        operator Ionospheric_UTC_Parameters() const {
          Ionospheric_UTC_Parameters converted;
#define CONVERT(TARGET) \
{converted.TARGET = sf[SF_ ## TARGET] * TARGET;}
#define CONVERT2(dst, src) \
{converted.dst = sf[SF_ ## src] * src;}
            CONVERT2(alpha[0], alpha0);
            CONVERT2(alpha[1], alpha1);
            CONVERT2(alpha[2], alpha2);
            CONVERT2(alpha[3], alpha3);
            CONVERT2(beta[0],  beta0);
            CONVERT2(beta[1],  beta1);
            CONVERT2(beta[2],  beta2);
            CONVERT2(beta[3],  beta3);
            CONVERT(A1);
            CONVERT(A0);
            converted.t_ot = ((uint_t)t_ot) << 12;
            converted.WN_t = WN_t;
            converted.delta_t_LS = delta_t_LS;
            converted.WN_LSF = WN_LSF;
            converted.DN = DN;
            converted.delta_t_LSF = delta_t_LSF;
#undef CONVERT
#undef CONVERT2
          return converted;
        };
      };
    };
  public:

    class SatelliteProperties {
      public:
        struct constellation_t {
          xyz_t position;
          xyz_t velocity;
        };
        
        /**
         * GPS ephemeris
         * (Subframe 1,2,3)
         * 
         */
        struct Ephemeris {
          uint_t svid;        ///< Satellite number
          
          // Subframe.1
          uint_t WN;          ///< Week number
          int_t URA;          ///< User range accuracy (index)
          uint_t SV_health;   ///< Health status
          int_t iodc;         ///< Issue of clock data
          float_t t_GD;       ///< Group delay (s)
          float_t t_oc;       ///< Clock data reference time
          float_t a_f2;       ///< Clock correction parameter (s/s^2)
          float_t a_f1;       ///< Clock correction parameter (s/s)
          float_t a_f0;       ///< Clock correction parameter (s)
          
          // Subframe.2
          int_t iode;           ///< Issue of ephemeris data
          float_t c_rs;         ///< Sine correction, orbit (m)
          float_t delta_n;      ///< Mean motion difference (rad/s)
          float_t M0;           ///< Mean anomaly (rad)
          float_t c_uc;         ///< Cosine correction, latitude (rad)
          float_t e;            ///< Eccentricity
          float_t c_us;         ///< Sine correction, latitude (rad)
          float_t sqrt_A;       ///< Square root of semi-major axis (sqrt(m))
          float_t t_oe;         ///< Reference time ephemeris (s)
          float_t fit_interval; ///< Fit interval; if negative, it indicates invalid ephemeris.
          
          // Subframe.3
          float_t c_ic;         ///< Cosine correction, inclination (rad)
          float_t Omega0;       ///< Longitude of ascending node (rad)
          float_t c_is;         ///< Sine correction, inclination (rad)
          float_t i0;           ///< Inclination angle (rad)
          float_t c_rc;         ///< Cosine correction, orbit (m)
          float_t omega;        ///< Argument of perigee (rad)
          float_t dot_Omega0;   ///< Rate of right ascension (rad/s)
          float_t dot_i0;       ///< Rate of inclination angle (rad/s)

          inline float_t period_from_time_of_clock(const gps_time_t &t) const {
            return -t.interval(WN, t_oc);
          }

          inline float_t period_from_time_of_ephemeris(const gps_time_t &t) const {
            return -t.interval(WN, t_oe);
          }
          
          /**
           * @return (float_t) if valid ephemeris, the return value is always positive,
           * because (t - t_oc), and (t - t_oe) in equations are normally negative.
           * @see 20.3.4.5, Table 20-XIII
           */
          inline float_t period_from_first_valid_transmittion(const gps_time_t &t) const {
            return period_from_time_of_clock(t) + (fit_interval / 2);
          }

          /**
           * Return true when valid
           * 
           * @param t GPS time
           */
          bool is_valid(const gps_time_t &t) const {
            return std::abs(period_from_time_of_clock(t)) <= (fit_interval / 2);
          }

          /**
           * Return true when newer ephemeris may be available
           * 
           * @param t GPS time
           */
          bool maybe_better_one_avilable(const gps_time_t &t) const {
            float_t delta_t(period_from_first_valid_transmittion(t));
            float_t transmittion_interval( // @see IDC 20.3.4.5 Reference Times, Table 20-XIII
                (fit_interval > (4 * 60 * 60))
                  ? fit_interval / 2 // fit_interval is more than 4 hour, fit_interval / 2
                  : (1 * 60 * 60));  // fit_interval equals to 4 hour, some SVs transmits every one hour.
            return !((delta_t >= 0) && (delta_t < transmittion_interval));
          }
          
          static const float_t URA_limits[];
          static const int URA_MAX_INDEX;

          static float_t URA_meter(const int_t &index){
            if(index < 0){return -1;}
            return (index < URA_MAX_INDEX)
                ? URA_limits[index]
                : URA_limits[URA_MAX_INDEX - 1] * 2;
          }

          static int_t URA_index(const float_t &meter){
            if(meter < 0){return -1;}
            for(int i(0); i < URA_MAX_INDEX; ++i){
              if(meter <= URA_limits[i]){return i;}
            }
            return URA_MAX_INDEX;
          }

          float_t eccentric_anomaly(const float_t &period_from_toe) const {

            // Kepler's Equation for Eccentric Anomaly M(Mk)
            float_t n0(std::sqrt(WGS84::mu_Earth) / pow3(sqrt_A));
            float_t Mk(M0
                + (n0 + delta_n) * period_from_toe);

            // Eccentric Anomaly E(Ek)
            float_t Ek(Mk);
  #ifndef KEPLER_DELTA_LIMIT
  #define KEPLER_DELTA_LIMIT 1E-12
  #endif
            for(int loop(0); loop < 10; loop++){
              float_t Ek2(Mk + e * sin(Ek));
              if(std::abs(Ek2 - Ek) < KEPLER_DELTA_LIMIT){break;}
              Ek = Ek2;
            }

            return Ek;
          }

          float_t eccentric_anomaly(const gps_time_t &t) const {
            return eccentric_anomaly(period_from_time_of_ephemeris(t));
          }

          float_t eccentric_anomaly_dot(const float_t &eccentric_anomaly) const {
            float_t n((std::sqrt(WGS84::mu_Earth) / pow3(sqrt_A)) + delta_n);
            return n / (1.0 - e * cos(eccentric_anomaly));
          }

          /**
           * Calculate correction value in accordance with clock error model
           *
           * @param t current time
           * @param pseudo_range pseudo range in meters
           * @param gamma factor for compensation of group delay
           * L1 = 1, L2 = (77/60)^2, see ICD 20.3.3.3.3.2 L1 - L2 Correction
           * @return in seconds
           */
          float_t clock_error(const gps_time_t &t, const float_t &pseudo_range = 0,
              const float_t &gamma = 1) const{

            float_t transit_time(pseudo_range / light_speed);
            float_t tk(period_from_time_of_clock(t) - transit_time);
            float_t Ek(eccentric_anomaly(tk));

            // Relativistic correction term
            static const float_t F(-2.0 * std::sqrt(WGS84::mu_Earth) / pow2(light_speed));
            float_t dt_r(F * e * sqrt_A * sin(Ek));

            float_t dt_sv(a_f0 + a_f1 * tk + a_f2 * pow2(tk) + dt_r); // ICD 20.3.3.3.1 Eq.(2)

            return dt_sv - (gamma * t_GD);
          }

          float_t clock_error_dot(const gps_time_t &t, const float_t &pseudo_range = 0) const {

            float_t transit_time(pseudo_range / light_speed);
            float_t tk(period_from_time_of_clock(t) - transit_time);
            float_t Ek(eccentric_anomaly(tk));
            float_t Ek_dot(eccentric_anomaly_dot(Ek));

            // Derivative of Relativistic correction term
            static const float_t F(-2.0 * std::sqrt(WGS84::mu_Earth) / pow2(light_speed));
            float_t dt_r_dot(F * e * sqrt_A * Ek_dot * cos(Ek));

            float_t dt_sv_dot(a_f1 + a_f2 * 2 * tk + dt_r_dot);

            return dt_sv_dot;
          }

          constellation_t constellation(
              const gps_time_t &t, const float_t &pseudo_range = 0,
              const bool &with_velocity = true) const {

            constellation_t res;

            // Time from ephemeris reference epoch (tk)
            float_t tk0(period_from_time_of_ephemeris(t));

            // Remove transit time
            float_t tk(tk0 - pseudo_range / light_speed);

            // Eccentric Anomaly (Ek)
            float_t Ek(eccentric_anomaly(tk));

            // Corrected Radius (rk)
            float_t rk(pow2(sqrt_A)
                * (1.0 - e * cos(Ek)));

            // True Anomaly (vk)
            float_t vk(atan2(
                sqrt(1.0 - pow2(e)) * sin(Ek),
                cos(Ek) - e));

            // (Corrected) Argument of Latitude (pk) [rad]
            float_t pk(vk + omega);

            // (Corrected) Inclination (ik)
            float_t ik(i0);

            { // Correction
              float_t pk2_sin(sin(pk * 2)),
                  pk2_cos(cos(pk * 2));
              float_t d_uk(
                  c_us * pk2_sin
                  + c_uc * pk2_cos);
              float_t d_rk(
                  c_rs * pk2_sin
                  + c_rc * pk2_cos);
              float_t d_ik(
                  c_is * pk2_sin
                  + c_ic * pk2_cos);

              pk += d_uk;
              rk += d_rk;
              ik += d_ik
                  + dot_i0 * tk;
            }

            // Position in orbital plane (xk, yk)
            float_t xk(rk * cos(pk)),
                yk(rk * sin(pk));

            // Corrected longitude of ascending node (Omegak) [rad]
            float_t Omegak(Omega0);
            if(false){ // __MISUNDERSTANDING_ABOUT_OMEGA0_CORRECTION__
              Omegak += (
                  (dot_Omega0 - WGS84::Omega_Earth_IAU) * tk0
                  - WGS84::Omega_Earth_IAU * t_oe);
            }else{
              Omegak += (
                  dot_Omega0 * tk                            // corrected with the time when the wave is transmitted
                  - WGS84::Omega_Earth_IAU * (t_oe + tk0));  // corrected with the time when the wave is received
            }

            float_t Omegak_sin(sin(Omegak)),
                   Omegak_cos(cos(Omegak));
            float_t ik_sin(sin(ik)),
                   ik_cos(cos(ik));

            res.position.x() = xk * Omegak_cos - yk * Omegak_sin * ik_cos;
            res.position.y() = xk * Omegak_sin + yk * Omegak_cos * ik_cos;
            res.position.z() = yk * ik_sin;

            // Velocity calculation => GPS solution vol.8 (3) http://www.ngs.noaa.gov/gps-toolbox/bc_velo.htm
            if(with_velocity){
              float_t Ek_dot(eccentric_anomaly_dot(Ek));
              float_t vk_dot(sin(Ek) * Ek_dot * (1.0 + e * cos(vk))
                  / (sin(vk) * (1.0 - e * cos(Ek))));

              float_t pk2_sin(sin(pk * 2)), pk2_cos(cos(pk * 2));
              float_t pk_dot(((c_us * pk2_cos - c_uc * pk2_sin) * 2 + 1.0) * vk_dot);
              float_t rk_dot(pow2(sqrt_A) * e * sin(Ek) * Ek_dot
                  + (c_rs * pk2_cos - c_rc * pk2_sin) * 2 * vk_dot);
              float_t ik_dot(dot_i0 + (c_is * pk2_cos - c_ic * pk2_sin) * 2 * vk_dot);

              // Velocity in orbital plane (xk_dot, yk_dot)
              float_t xk_dot(rk_dot * cos(pk) - yk * pk_dot),
                  yk_dot(rk_dot * sin(pk) + xk * pk_dot);

              float_t Omegak_dot(dot_Omega0 - WGS84::Omega_Earth_IAU);

              res.velocity.x() = (xk_dot - yk * ik_cos * Omegak_dot) * Omegak_cos
                  - (xk * Omegak_dot + yk_dot * ik_cos - yk * ik_sin * ik_dot) * Omegak_sin;
              res.velocity.y() = (xk_dot - yk * ik_cos * Omegak_dot) * Omegak_sin
                  + (xk * Omegak_dot + yk_dot * ik_cos - yk * ik_sin * ik_dot) * Omegak_cos;
              res.velocity.z() = yk_dot * ik_sin + yk * ik_cos * ik_dot;
            }

            return res;
          }

          struct raw_t {
            u8_t  svid;         ///< Satellite number
            
            u16_t WN;           ///< Week number
            u8_t  URA;          ///< User range accuracy
            u8_t  SV_health;    ///< Health status
            u16_t iodc;         ///< Issue of clock data
            s8_t  t_GD;         ///< Group delay (-31, s)
            u16_t t_oc;         ///< Clock data reference time
            s8_t  a_f2;         ///< Clock correction parameter (-55, s/s^2)
            s16_t a_f1;         ///< Clock correction parameter (-43, s/s)
            s32_t a_f0;         ///< Clock correction parameter (-31, s)
            
            u8_t  iode;         ///< Issue of eph. data
            s16_t c_rs;         ///< Sin. correction, orbit ( -5, m)
            s16_t delta_n;      ///< Mean motion difference (-43, sc/s)
            s32_t M0;           ///< Mean anomaly           (-31, sc)
            s16_t c_uc;         ///< Cos. correction, lat.  (-29, rad)
            u32_t e;            ///< Eccentricity           (-33)
            s16_t c_us;         ///< Sin. correction, lat.  (-29, rad)
            u32_t sqrt_A;       ///< Root semi-Major Axis   (-19, sqrt(m))
            u16_t t_oe;         ///< Reference time eph.    (  4, s)
            bool fit_interval_flag;   ///< Fit interval flag (ICD:20.3.4.4)
            
            s16_t c_ic;         ///< Cos. correction, incl. (-29, rad)
            s32_t Omega0;       ///< Ascending node long.   (-31, sc)
            s16_t c_is;         ///< Sin. correction, incl. (-29, rad)
            s32_t i0;           ///< Inclination angle      (-31, sc)
            s16_t c_rc;         ///< Cos. correction, orbit ( -5, m)
            s32_t omega;        ///< Argument of perigee    (-31, sc)
            s32_t dot_Omega0;   ///< Right ascension rate   (-43, sc/s)
            s16_t dot_i0;       ///< Inclination angle rate (-43, sc/s)

#define fetch_item(num, name) name = BroadcastedMessage< \
   InputT, (int)sizeof(InputT) * CHAR_BIT - PaddingBits_MSB - PaddingBits_LSB, PaddingBits_MSB> \
   :: SubFrame ## num :: name (src)
            template <int PaddingBits_MSB, int PaddingBits_LSB, class InputT>
            u16_t update_subframe1(const InputT *src){
              fetch_item(1, WN);
              fetch_item(1, URA);
              fetch_item(1, SV_health);
              fetch_item(1, iodc);
              fetch_item(1, t_GD);
              fetch_item(1, t_oc);
              fetch_item(1, a_f2);
              fetch_item(1, a_f1);
              fetch_item(1, a_f0);
              return iodc;
            }
            template <int PaddingBits_MSB, int PaddingBits_LSB, class InputT>
            u8_t update_subframe2(const InputT *src){
              fetch_item(2, iode);
              fetch_item(2, c_rs);
              fetch_item(2, delta_n);
              fetch_item(2, M0);
              fetch_item(2, c_uc);
              fetch_item(2, e);
              fetch_item(2, c_us);
              fetch_item(2, sqrt_A);
              fetch_item(2, t_oe);
              u8_t fetch_item(2, fit); fit_interval_flag = (fit == 1);
              return iode;
            }
            template <int PaddingBits_MSB, int PaddingBits_LSB, class InputT>
            u8_t update_subframe3(const InputT *src){
              fetch_item(3, c_ic);
              fetch_item(3, Omega0);
              fetch_item(3, c_is);
              fetch_item(3, i0);
              fetch_item(3, c_rc);
              fetch_item(3, omega);
              fetch_item(3, dot_Omega0);
              fetch_item(3, dot_i0);
              fetch_item(3, iode);
              return iode;
            }
#undef fetch_item

            static float_t fit_interval(const bool &_flag, const u16_t &_iodc){
              // Fit interval (ICD:20.3.4.4)
              if(_flag == false){
                // normal operation
                return 4 * 60 * 60;
              }else{
                // short/long-term extended operation
                if(_iodc >= 240 && _iodc <= 247){
                  return 8 * 60 * 60;
                }else if((_iodc >= 248 && _iodc <= 255) || (_iodc == 496)){
                  return 14 * 60 * 60;
                }else if(_iodc >= 497 && _iodc <= 503){
                  return 26 * 60 * 60;
                }else if(_iodc >= 504 && _iodc <= 510){
                  return 50 * 60 * 60;
                }else if((_iodc == 511) || (_iodc >= 752 && _iodc <= 756)){
                  return 74 * 60 * 60;
                }else if(_iodc >= 757 && _iodc <= 763){
                  return 98 * 60 * 60;
                }else if((_iodc >= 764 && _iodc <= 767) || (_iodc >= 1008 && _iodc <= 1010)){
                  return 122 * 60 * 60;
                }else if(_iodc >= 1011 && _iodc <= 1020){
                  return 146 * 60 * 60;
                }else{
                  return 6 * 60 * 60;
                }
              }
            }

            enum {
              SF_t_GD,
              SF_t_oc,
              SF_a_f0,
              SF_a_f1,
              SF_a_f2,

              SF_c_rs,
              SF_delta_n,
              SF_M0,
              SF_c_uc,
              SF_e,
              SF_c_us,
              SF_sqrt_A,
              SF_t_oe,

              SF_c_ic,
              SF_Omega0,
              SF_c_is,
              SF_i0,
              SF_c_rc,
              SF_omega,
              SF_dot_Omega0,
              SF_dot_i0,

              SF_NUM,
            };
            static const float_t sf[SF_NUM];

            operator Ephemeris() const {
              Ephemeris converted;
#define CONVERT(TARGET) \
{converted.TARGET = sf[SF_ ## TARGET] * TARGET;}
              converted.svid = svid;
              
              converted.WN = WN;
              converted.URA = URA;
              converted.SV_health = SV_health;
              converted.iodc = iodc;
              CONVERT(t_GD);
              CONVERT(t_oc);
              CONVERT(a_f0);
              CONVERT(a_f1);
              CONVERT(a_f2);
              
              converted.iode = iode;
              CONVERT(c_rs);
              CONVERT(delta_n);
              CONVERT(M0);
              CONVERT(c_uc);
              CONVERT(e);
              CONVERT(c_us);
              CONVERT(sqrt_A);
              CONVERT(t_oe);
              
              CONVERT(c_ic);
              CONVERT(Omega0);
              CONVERT(c_is);
              CONVERT(i0);
              CONVERT(c_rc);
              CONVERT(omega);
              CONVERT(dot_Omega0);
              CONVERT(dot_i0);
#undef CONVERT
              converted.fit_interval = fit_interval(fit_interval_flag, iodc);
              
              return converted;
            }

            raw_t &operator=(const Ephemeris &eph){
#define CONVERT(TARGET) \
{TARGET = (s32_t)((eph.TARGET + 0.5 * sf[SF_ ## TARGET]) / sf[SF_ ## TARGET]);}
              svid = eph.svid;

              WN = eph.WN;
              URA = eph.URA;
              SV_health = eph.SV_health;
              iodc = eph.iodc;
              CONVERT(t_GD);
              CONVERT(t_oc);
              CONVERT(a_f0);
              CONVERT(a_f1);
              CONVERT(a_f2);

              iode = eph.iode;
              CONVERT(c_rs);
              CONVERT(delta_n);
              CONVERT(M0);
              CONVERT(c_uc);
              CONVERT(e);
              CONVERT(c_us);
              CONVERT(sqrt_A);
              CONVERT(t_oe);

              CONVERT(c_ic);
              CONVERT(Omega0);
              CONVERT(c_is);
              CONVERT(i0);
              CONVERT(c_rc);
              CONVERT(omega);
              CONVERT(dot_Omega0);
              CONVERT(dot_i0);
#undef CONVERT
              fit_interval_flag = (eph.fit_interval > 5 * 60 * 60);

              return *this;
            }
          };

          bool is_equivalent(const Ephemeris &eph) const {
            do{
              if(WN != eph.WN){break;}
              if(URA != eph.URA){break;}
              if(SV_health != eph.SV_health){break;}

#define CHECK(TARGET) \
if(std::abs(TARGET - eph.TARGET) > raw_t::sf[raw_t::SF_ ## TARGET]){break;}
              CHECK(t_GD);
              CHECK(t_oc);
              CHECK(a_f2);
              CHECK(a_f1);
              CHECK(a_f0);

              CHECK(c_rs);
              CHECK(delta_n);
              CHECK(M0);
              CHECK(c_uc);
              CHECK(e);
              CHECK(c_us);
              CHECK(sqrt_A);
              CHECK(t_oe);

              CHECK(c_ic);
              CHECK(Omega0);
              CHECK(c_is);
              CHECK(i0);
              CHECK(c_rc);
              CHECK(omega);
              CHECK(dot_Omega0);
              CHECK(dot_i0);
#undef CHECK
              return true;
            }while(false);
            return false;
          }

          gps_time_t base_time() const {
            return gps_time_t(WN, t_oc);
          }
        };

        /**
         * GPS almanac
         * (Subframe 4,5)
         * 
         */
        struct Almanac {
          uint_t  svid;         ///< Satellite number
          
          float_t e;            ///< Eccentricity
          float_t t_oa;         ///< Almanac reference time (s)
          float_t delta_i;      ///< Correction to inclination (rad)
          float_t dot_Omega0;   ///< Omega0 rate (rad/s)
          uint_t SV_health;     ///< Health status
          float_t sqrt_A;       ///< Square root of semi-major axis (sqrt(m))
          float_t Omega0;       ///< Longitude of ascending node (rad)
          float_t omega;        ///< Argument of perigee (rad)
          float_t M0;           ///< Mean anomaly (rad)
          float_t a_f0;         ///< Clock correction parameter (s/s)
          float_t a_f1;         ///< Clock correction parameter (s)
          
          /**
           * Up-cast to ephemeris
           * 
           */
          operator Ephemeris() const {
            Ephemeris converted;
            
            converted.svid          = svid;
            
            // Subframe.1
            converted.WN            = 0;          // Week number (must be configured later)
            converted.URA           = -1;         // User range accuracy
            converted.SV_health     = SV_health;  // Health status
            converted.iodc          = -1;         // Issue of clock data
            converted.t_GD          = 0;          // Group delay (s)
            converted.t_oc          = t_oa;       // Clock data reference time
            converted.a_f2          = 0;          // Clock correction parameter (s/s^2)
            converted.a_f1          = a_f1;       // Clock correction parameter (s/s)
            converted.a_f0          = a_f0;       // Clock correction parameter (s)
            
            // Subframe.2
            converted.iode          = -1;         // Issue of ephemeris data
            converted.c_rs          = 0;          // Sine correction, orbit (m)
            converted.delta_n       = 0;          // Mean motion difference (rad/s)
            converted.M0            = M0;         // Mean anomaly (rad)
            converted.c_uc          = 0;          // Cosine correction, latitude (rad)
            converted.e             = e;          // Eccentricity
            converted.c_us          = 0;          // Sine correction, latitude (rad)
            converted.sqrt_A        = sqrt_A;     // Square root of semi-major axis (sqrt(m))
            converted.t_oe          = t_oa;       // Reference time ephemeris (s)
            converted.fit_interval  = 4 * 60 * 60;// Fit interval
            
            // Subframe.3
            converted.c_ic          = 0;          // Cosine correction, inclination (rad)
            converted.Omega0        = Omega0;     // Longitude of ascending node (rad)
            converted.c_is          = 0;          // Sine correction, inclination (rad)
            converted.i0            = delta_i;    // Inclination angle (rad)
            converted.c_rc          = 0;          // Cosine correction, orbit (m)
            converted.omega         = omega;      // Argument of perigee (rad)
            converted.dot_Omega0    = dot_Omega0; // Rate of right ascension (rad/s)
            converted.dot_i0        = 0;          // Rate of inclination angle (rad/s)
            
            return converted;
          }
          
          struct raw_t {
            u8_t  svid;         ///< Satellite number
            
            u16_t e;            ///< Eccentricity       (-21)
            u8_t  t_oa;         ///< Almanac ref. time  ( 12, s)
            s16_t delta_i;      ///< Correction to inc. (-19, sc)
            s16_t dot_Omega0;   ///< Omega0 rate        (-38, sc/s)
            u8_t  SV_health;    ///< Health status
            u32_t sqrt_A;       ///< Semi-major axis    (-11, sqrt(m))
            s32_t Omega0;       ///< Long. of asc. node (-23, sc)
            s32_t omega;        ///< Arg. of perigee    (-23, sc)
            s32_t M0;           ///< Mean anomaly       (-23, sc)
            s16_t a_f0;         ///< Clock corr. param. (-20, s)
            s16_t a_f1;         ///< Clock corr. param. (-38, s)
            
#define fetch_item(name) name = BroadcastedMessage< \
   InputT, (int)sizeof(InputT) * CHAR_BIT - PaddingBits_MSB - PaddingBits_LSB, PaddingBits_MSB> \
   :: SubFrame4_5_Alnamac :: name (src)
            template <int PaddingBits_MSB, int PaddingBits_LSB, class InputT>
            void update(const InputT *src){
              fetch_item(e);
              fetch_item(t_oa);
              fetch_item(delta_i);
              fetch_item(dot_Omega0);
              fetch_item(SV_health);
              fetch_item(sqrt_A);
              fetch_item(Omega0);
              fetch_item(omega);
              fetch_item(M0);
              fetch_item(a_f0);
              fetch_item(a_f1);
            }
#undef fetch_item

            enum {
              SF_e,
              SF_t_oa,
              SF_delta_i,
              SF_dot_Omega0,
              SF_sqrt_A,
              SF_Omega0,
              SF_omega,
              SF_M0,
              SF_a_f0,
              SF_a_f1,

              SF_NUM,
            };
            static const float_t sf[SF_NUM];

            operator Almanac() const {
              Almanac converted;
#define CONVERT(TARGET) \
{converted.TARGET = sf[SF_ ## TARGET] * TARGET;}
                converted.svid = svid;
                CONVERT(e);
                CONVERT(t_oa);
                CONVERT(delta_i);
                CONVERT(dot_Omega0);
                converted.SV_health = SV_health;
                CONVERT(sqrt_A);
                CONVERT(Omega0);
                CONVERT(omega);
                CONVERT(M0);
                CONVERT(a_f0);
                CONVERT(a_f1);
#undef CONVERT
              return converted;
            }
          };
        };
    };

    template <
        class PropertyT = typename SatelliteProperties::Ephemeris,
        int TimeQuantization = 10>
    class PropertyHistory {
      protected:
        struct item_t : public PropertyT {
          int priority;
          int_t t_tag; ///< time tag calculated with base_time()

          static int calc_t_tag(const float_t &t){
            float_t res(std::floor((t + (0.5 * TimeQuantization)) / TimeQuantization));
            if(res >= INT_MAX){return INT_MAX;}
            if(res <= INT_MIN){return INT_MIN;}
            return (int)res;
          }
          static int calc_t_tag(const gps_time_t &t){
            return calc_t_tag(t.serialize());
          }
          static int calc_t_tag(const PropertyT &prop){
            return calc_t_tag(prop.base_time());
          }

          item_t() : priority(0), t_tag(0) {}
          item_t(const PropertyT &prop)
              : PropertyT(prop), priority(0), t_tag(calc_t_tag(prop)) {}
          item_t(const PropertyT &prop, const int &priority_init)
              : PropertyT(prop), priority(priority_init), t_tag(calc_t_tag(prop)) {}
          item_t &operator=(const PropertyT &prop){
            PropertyT::operator=(prop);
            t_tag = calc_t_tag(prop);
            return *this;
          }

          bool operator==(const PropertyT &prop){
            return PropertyT::is_equivalent(prop);
          }
        };
        typedef std::vector<item_t> history_t;
        history_t history; // listed in chronological and higher priority order
        typename history_t::size_type selected_index;

        typename history_t::iterator selected_iterator() {
          typename history_t::iterator it(history.begin());
          std::advance(it, selected_index);
          return it;
        }

      public:
        PropertyHistory() : history(1), selected_index(0) {}

        enum each_mode_t {
          EACH_ALL,
          EACH_ALL_INVERTED,
          EACH_NO_REDUNDANT,
        };

        /**
         * Iterate each item.
         *
         * @param mode If EACH_ALL, all items will be passed, and when multiple items have same t_tag,
         * their order will be unchanged (highest to lowest priorities).
         * If EACH_ALL_INVERTED, all items will be passed, and when multiple items have same t_tag,
         * their order will be inverted (lowest to highest priorities).
         * If EACH_NO_REDUNDANT, when multiple items have same t_tag,
         * then, only item which has highest priority will be passed.
         */
        template <class Functor>
        void each(
            Functor &functor,
            const each_mode_t &mode = EACH_ALL) const {
          switch(mode){
            case EACH_ALL_INVERTED: {
              typename history_t::const_iterator it(history.begin() + 1);
              while(it != history.end()){
                typename history_t::const_iterator it2(it + 1);
                while((it2 != history.end()) && (it->t_tag == it2->t_tag)){
                  ++it2;
                }
                typename history_t::const_iterator it_next(it2);
                do{
                  functor(*(--it2));
                }while(it2 != it);
                it = it_next;
              }
              break;
            }
            case EACH_NO_REDUNDANT: {
              int_t t_tag(0);
              for(typename history_t::const_iterator it(history.begin() + 1);
                  it != history.end();
                  ++it){
                if(t_tag == it->t_tag){continue;}
                functor(*it);
                t_tag = it->t_tag;
              }
              break;
            }
            case EACH_ALL:
            default:
              for(typename history_t::const_iterator it(history.begin() + 1);
                  it != history.end();
                  ++it){
                functor(*it);
              }
              break;
          }
        }

        /**
         * Add new item
         *
         * @param item new item, assuming the latest one
         * @param priority_delta If the new item is already registered,
         * its priority will be increased by priority_delta.
         * If priority_delta == 0, the previous item is replaced to the new item.
         */
        void add(const PropertyT &item, const int &priority_delta = 1){
          int t_tag_new(item_t::calc_t_tag(item));
          typename history_t::iterator it_insert(history.begin());

          for(typename history_t::reverse_iterator it(history.rbegin());
              it != history.rend();
              ++it){

            int delta_t_tag(t_tag_new - it->t_tag);
            if(delta_t_tag < 0){continue;} // adding item is older.

            it_insert = it.base();
            if(delta_t_tag > 0){break;} // adding item is newer.

            while(true){ // Loop for items having the same time stamp; delta_t_tag == 0
              if(!(it->is_equivalent(item))){
                if(it->priority <= priority_delta){
                  it_insert = it.base() - 1;
                }
                if(((++it) == history.rend()) || (it->t_tag < t_tag_new)){break;}
                continue;
              }

              // When the content is equivalent
              if(priority_delta == 0){ // replace to newer one
                *it = item;
                return;
              }

              int rel_pos(selected_index - (std::distance(history.begin(), it.base()) - 1));
              int shift(0);
              (it->priority) += priority_delta;
              item_t copy(*it);

              if(priority_delta > 0){ // priority increased, thus move backward
                for(typename history_t::reverse_iterator it_previous(it + 1);
                    it_previous != history.rend()
                      && (it_previous->t_tag == t_tag_new)
                      && (it_previous->priority <= copy.priority); // if priority is same or higher, then swap.
                    ++it, ++it_previous, --shift){
                  *it = *it_previous;
                }
                if(shift != 0){*it = copy;} // moved
              }else{ // priority decreased, thus move forward
                typename history_t::iterator it2(it.base() - 1);
                for(typename history_t::iterator it2_next(it2 + 1);
                    it2_next != history.end()
                      && (it2_next->t_tag == t_tag_new)
                      && (it2_next->priority > copy.priority); // if priority is lower, then swap.
                    ++it2, ++it2_next, ++shift){
                  *it2 = *it2_next;
                }
                if(shift != 0){*it2 = copy;} // moved
              }

              if(rel_pos == 0){ // When the added item is selected
                selected_index += shift;
              }else if((rel_pos < 0) && (shift <= rel_pos)){
                // When selected item has same time stamp and different content, and its priority is higher.
                selected_index++;
              }else if((rel_pos > 0) && (shift >= rel_pos)){
                // When selected item has same time stamp and different content, and its priority is higher.
                selected_index--;
              }

              return;
            }

            break; // Reach only when having the same time stamp, but different content
          }

          // insert new one.
          if(std::distance(history.begin(), it_insert) < (int)selected_index){
            selected_index++;
          }
          history.insert(it_insert, item_t(item, priority_delta));
        }

        /**
         * Select best valid item among registered ones.
         *
         * @param target_time time at measurement
         * @param is_valid function to check validity subject to specific time
         * @param get_delta_t function to get time difference (delta_t).
         * When NULL (default), time tag is used to calculate delta_t.
         * @return if true, a better valid item is newly selected; otherwise false.
         */
        bool select(
            const gps_time_t &target_time,
            bool (PropertyT::*is_valid)(const gps_time_t &) const,
            float_t (PropertyT::*get_delta_t)(const gps_time_t &) const = NULL){
          typename history_t::iterator it_selected(selected_iterator());

          bool changed(false);

          int t_tag(it_selected->t_tag);
          int t_tag_target(item_t::calc_t_tag(target_time));
          float_t delta_t(get_delta_t
              ? ((*it_selected).*get_delta_t)(target_time)
              : (t_tag_target - t_tag));

          typename history_t::iterator it, it_last;
          if(delta_t >= 0){
            // find newer
            it = it_selected + 1;
            it_last = history.end();
          }else{
            // find older (rare case, slow)
            it = history.begin();
            it_last = it_selected;
            delta_t *= -1;
          }

          /* Selection priority:
           * Valid item having higher priority and less abs(delta_t),
           * which means when an item has been selected,
           * the others having same time tag will be skipped.
           */
          for( ; it != it_last; ++it){
            if(changed && (t_tag == it->t_tag)){continue;} // skip one having same time tag, because highest priority one is selected.
            if(!(((*it).*is_valid)(target_time))){continue;}
            float_t delta_t2(get_delta_t
                ? ((*it).*get_delta_t)(target_time)
                : (t_tag_target - it->t_tag));
            if(delta_t2 < 0){delta_t2 *= -1;}
            if(delta_t > delta_t2){ // update
              changed = true;
              t_tag = it->t_tag;
              delta_t = delta_t2;
              selected_index = std::distance(history.begin(), it);
            }
          }

          return changed;
        }

        /**
         * Merge information
         */
        void merge(const PropertyHistory &another, const bool &keep_original = true){
          history_t list_new;
          list_new.push_back(history[0]);
          typename history_t::const_iterator
              it1(history.begin() + 1), it2(another.history.begin() + 1);
          typename history_t::size_type current_index_new(selected_index);
          int shift_count(selected_index - 1);
          while(true){
            if(it1 == history.end()){
              while(it2 != another.history.end()){
                list_new.push_back(*it2);
              }
              break;
            }else if(it2 == another.history.end()){
              while(it1 != history.end()){
                list_new.push_back(*it1);
              }
              break;
            }
            int delta_t(it1->t_tag - it2->t_tag);
            bool use_it1(true);
            if(delta_t == 0){
              if(it1->is_equivalent(*it2)){
                list_new.push_back(keep_original ? *it1 : *it2);
                ++it1;
                ++it2;
                --shift_count;
                continue;
              }else if(it1->priority < it2->priority){
                use_it1 = false;
              }
            }else if(delta_t > 0){
              use_it1 = false;
            }

            if(use_it1){
              list_new.push_back(*it1);
              ++it1;
              --shift_count;
            }else{
              list_new.push_back(*it2);
              ++it2;
              if(shift_count >= 0){++current_index_new;}
            }
          }
          history = list_new;
          selected_index = current_index_new;
        }

        const PropertyT &current() const {
          return history[selected_index];
        }
    };

    class Satellite : public SatelliteProperties {
      public:
        typedef typename SatelliteProperties::Ephemeris eph_t;
        typedef PropertyHistory<eph_t> eph_list_t;
      protected:
        eph_list_t eph_history;
      public:
        Satellite() : eph_history() {
          // setup first ephemeris as invalid one
          eph_t &eph_current(const_cast<eph_t &>(eph_history.current()));
          eph_current.WN = 0;
          eph_current.t_oc = 0;
          eph_current.t_oe = 0;
          eph_current.fit_interval = -1;
        }

        template <class Functor>
        void each_ephemeris(
            Functor &functor,
            const typename eph_list_t::each_mode_t &mode = eph_list_t::EACH_ALL) const {
          eph_history.each(functor, mode);
        }

        void register_ephemeris(const eph_t &eph, const int &priority_delta = 1){
          eph_history.add(eph, priority_delta);
        }

        void merge(const Satellite &another, const bool &keep_original = true){
          eph_history.merge(another, keep_original);
        }

        const eph_t &ephemeris() const {
          return eph_history.current();
        }

        /**
         * Select appropriate ephemeris within registered ones.
         *
         * @param target_time time at measurement
         * @return if true, appropriate ephemeris is selected, otherwise, not selected.
         */
        bool select_ephemeris(const gps_time_t &target_time){
          bool is_valid(ephemeris().is_valid(target_time));
          if(is_valid && (!ephemeris().maybe_better_one_avilable(target_time))){
            return true; // conservative
          }
          return eph_history.select(
              target_time,
              &eph_t::is_valid,
              &eph_t::period_from_first_valid_transmittion) || is_valid;
        }

        float_t clock_error(const gps_time_t &t, const float_t &pseudo_range = 0) const{
          return ephemeris().clock_error(t, pseudo_range);
        }

        float_t clock_error_dot(const gps_time_t &t, const float_t &pseudo_range = 0) const {
          return ephemeris().clock_error_dot(t, pseudo_range);
        }

        typename SatelliteProperties::constellation_t constellation(
            const gps_time_t &t, const float_t &pseudo_range = 0,
            const bool &with_velocity = true) const {
          return ephemeris().constellation(t, pseudo_range, with_velocity);
        }
        
        xyz_t position(const gps_time_t &t, const float_t &pseudo_range = 0) const {
          return constellation(t, pseudo_range, false).position;
        }
        
        xyz_t velocity(const gps_time_t &t, const float_t &pseudo_range = 0) const {
          return constellation(t, pseudo_range, true).velocity;
        }
    };
  public:
    typedef std::map<int, Satellite> satellites_t;
  protected:
    Ionospheric_UTC_Parameters _iono_utc;
    bool _iono_initialized, _utc_initialized;
    satellites_t _satellites;
  public:
    GPS_SpaceNode()
        : _iono_initialized(false), _utc_initialized(false),
        _satellites() {
    }
    ~GPS_SpaceNode(){
      _satellites.clear();
    }
    const Ionospheric_UTC_Parameters &iono_utc() const {
      return _iono_utc;
    }
    bool is_valid_iono() const {
      return _iono_initialized;
    }
    bool is_valid_utc() const {
      return _utc_initialized;
    }
    bool is_valid_iono_utc() const {
      return is_valid_iono() && is_valid_utc();
    }
    const Ionospheric_UTC_Parameters &update_iono_utc(
        const Ionospheric_UTC_Parameters &params,
        const bool &iono_valid = true,
        const bool &utc_valid = true) {
      _iono_initialized = iono_valid;
      _utc_initialized = utc_valid;
      return (_iono_utc = params);
    }

    const satellites_t &satellites() const {
      return _satellites;
    }
    Satellite &satellite(const int &prn) {
      return _satellites[prn];
    }
    bool has_satellite(const int &prn) const {
      return _satellites.find(prn) !=  _satellites.end();
    }
    void update_all_ephemeris(const gps_time_t &target_time) {
      for(typename satellites_t::iterator it(_satellites.begin());
          it != _satellites.end(); ++it){
        it->second.select_ephemeris(target_time);
      }
    }
    void merge(const self_t &another, const bool &keep_original = true){
      for(typename satellites_t::const_iterator it(another._satellites.begin());
          it != another._satellites.end();
          ++it){
        satellite(it->first).merge(it->second, keep_original);
      }
      if((!is_valid_iono_utc()) || (!keep_original)){
        _iono_utc = another._iono_utc;
        _iono_initialized = another._iono_initialized;
        _utc_initialized = another._utc_initialized;
      }
    }

    /**
     * Calculate pierce point position
     *
     * @param relative_pos satellite position (relative position, NEU)
     * @param usrllh user position (absolute position, LLH)
     * @param height_over_ellipsoid
     * @see DO-229D A4.4.10.1 Pierce Point Location Determination
     */
    struct pierce_point_res_t {
      float_t latitude, longitude;
    };
    static pierce_point_res_t pierce_point(
        const enu_t &relative_pos,
        const llh_t &usrllh,
        const float_t &height_over_ellipsoid = 350E3) {
      float_t el(relative_pos.elevation()),
           az(relative_pos.azimuth());
      // Earth's central angle between use pos. and projection of PP.
      float_t psi_pp(M_PI / 2 - el
          - std::asin(WGS84::R_e / (WGS84::R_e + height_over_ellipsoid) * std::cos(el)));
      float_t phi_pp(
          std::asin(std::sin(usrllh.latitude() * std::cos(psi_pp)
            + std::cos(usrllh.latitude()) * std::sin(psi_pp) * std::cos(az)))); // latitude
      float_t lambda_pp_last(
          std::asin(std::sin(psi_pp) * std::sin(az) / std::cos(phi_pp)));

      pierce_point_res_t res;
      res.latitude = phi_pp;
      {
        const float_t phi_limit(std::asin(WGS84::R_e / (WGS84::R_e + height_over_ellipsoid)));
        // Check whether longitude is opposite side.
        // This is possible when pierce point is located on the horizontal plane.
        // If pierce point height is 350km, the limit latitude yields asin(Re / (350E3 + Re)) = 71.4 [deg].
        float_t lhs(std::tan(psi_pp) * std::cos(az)), rhs(std::tan(M_PI / 2 - usrllh.latitude()));
        if(((usrllh.latitude() > phi_limit) && (lhs > rhs))
            || ((usrllh.latitude() < -phi_limit) & (lhs < rhs))){
          res.longitude = usrllh.longitude() + M_PI - lambda_pp_last;
        }else{
          res.longitude = usrllh.longitude() + lambda_pp_last;
        }
      }
      return res;
    }

    /**
     * Calculate ratio of slant versus vertical
     *
     * @relative_pos satellite position (relative position, NEU)
     * @param height_over_ellipsoid
     * @see spherically single layer approach, for example,
     * Eq.(3) of "Ionospheric Range Error Correction Models" by N. Jakowski
     */
    static float_t slant_factor(
        const enu_t &relative_pos,
        const float_t &height_over_ellipsoid = 350E3) {
      return std::sqrt(
          -std::pow(std::cos(relative_pos.elevation()) / ((height_over_ellipsoid / WGS84::R_e) + 1), 2)
          + 1);
    }

    /**
     * Calculate ionospheric delay by using TEC
     *
     * @param tec TEC (total electron count)
     * @param freq frequency (default is L1 frequency)
     * @return delay (if delayed, positive number will be returned)
     * @see http://www.navipedia.net/index.php/Ionospheric_Delay Eq.(13)
     */
    static float_t tec2delay(const float_t &tec, const float_t &freq = L1_Frequency) {
      const float_t a_f(40.3E16 / std::pow(freq, 2));
      return a_f * tec;
    }

    /**
     * Calculate correction value in accordance with ionospheric model
     * 
     * @param relative_pos satellite position (relative position, NEU)
     * @param usrllh user position (absolute position, LLH)
     * @return correction in meters
     */
    float_t iono_correction(
        const enu_t &relative_pos,
        const llh_t &usrllh,
        const gps_time_t &t) const {
      
      
      // Elevation and azimuth
      float_t el(relative_pos.elevation()),
             az(relative_pos.azimuth());
      float_t sc_el(rad2sc(el)),
             sc_az(rad2sc(az));
             
      // Pierce point (PP stands for the earth projection of the Pierce point)
      // (the following equation is based on GPS ICD)
      float_t psi(0.0137 / (sc_el + 0.11) - 0.022); // central angle between user pos and PP.
      float_t phi_i(rad2sc(usrllh.latitude())
          + psi * cos(az)); // geodetic latitude of PP [sc]
      if(phi_i > 0.416){phi_i = 0.416;}
      else if(phi_i < -0.416){phi_i = -0.416;}
      float_t lambda_i(rad2sc(usrllh.longitude())
          + psi * sin(az) / cos(sc2rad(phi_i))); // geodetic longitude of PP [sc]
      float_t phi_m(phi_i
          + 0.064 * cos(sc2rad(lambda_i - 1.617))); // geomagnetic latitude of PP [sc]
      
      // Local time [sec]
      float_t lt(4.32E4 * lambda_i + t.seconds);
      lt -= std::floor(lt / gps_time_t::seconds_day) * gps_time_t::seconds_day; // lt = [0,86400)
      
      // Period and amplitude of cosine function
      float_t amp(0), per(0);
      float_t phi_mn(1.);
      for(int i(0); i < 4; i++){
        amp += _iono_utc.alpha[i] * phi_mn;
        per += _iono_utc.beta[i] * phi_mn;
        phi_mn *= phi_m;
      }
      if(amp < 0){amp = 0;}
      if(per < 72000){per = 72000;}
      
      // Obliquity factor
      float_t F(1.0 + 16.0 * pow((0.53 - sc_el), 3));
      
      float_t x(M_PI * 2 * (lt - 50400) / per); // phase [rad] => (-1.4 pi) < x < (0.42 pi) because min(per) = 72000
      //x -= M_PI * 2 * std::floor(x / (M_PI * 2) + 0.5); // x = [-pi,pi)
      
      float_t T_iono(5E-9);
      if(std::abs(x) < 1.57){
        T_iono += amp * (1. - pow2(x) * (1.0 / 2 - pow2(x) / 24)); // ICD p.148
      }
      T_iono *= F;
      
      return -T_iono * light_speed;
    }
    
    /**
     * Calculate correction value in accordance with ionospheric model
     *
     * @param sat satellite position (absolute position, XYZ)
     * @param usr user position (absolute position, XYZ)
     * @return correction in meters
     */
    float_t iono_correction(
        const xyz_t &sat,
        const xyz_t &usr,
        const gps_time_t &t) const {
      return iono_correction(
          enu_t::relative(sat, usr), 
          usr.llh(), 
          t);
    }
    
    struct IonospericCorrection {
      const GPS_SpaceNode<float_t> &space_node;
      float_t operator()(
          const enu_t &relative_pos,
          const llh_t &usrllh,
          const gps_time_t &t) const {
        return space_node.iono_correction(relative_pos, usrllh, t);
      }
      float_t operator()(
          const xyz_t &sat,
          const xyz_t &usr,
          const gps_time_t &t) const {
        return space_node.iono_correction(sat, usr, t);
      }
    };
    IonospericCorrection iono_correction() const {
      IonospericCorrection res = {*this};
      return res;
    }

    /**
     * Calculate correction value in accordance with tropospheric model
     * 
     * @param relative_pos satellite position (relative position, NEU)
     * @param usrllh user position (absolute position, LLH)
     * @return correction in meters
     */
    float_t tropo_correction(
        const enu_t &relative_pos,
        const llh_t &usrllh) const {
      
      // Elevation (rad)
      float_t el(relative_pos.elevation());
      
      // Altitude (m)
      const float_t &h(usrllh.height());
      float_t f(1.0);
      if(h > (1.0 / 2.3E-5)){
        f = 0;
      }else if(h > 0){
        f -= h * 2.3E-5;
      }
      
      return -2.47 * pow(f, 5) / (sin(el) + 0.0121);
    }
    
    struct NiellMappingFunction {
      float_t hydrostatic, wet;
      static float_t marini1972_2(const float_t &v, const float_t (&coef)[3]) {
        return (coef[0] / ((coef[1] / (coef[2] + v)) + v) + v);
      }
      static float_t marini1972(const float_t &sin_elv, const float_t (&coef)[3]) {
        return marini1972_2(1, coef) / marini1972_2(sin_elv, coef);
      }
      static NiellMappingFunction get(
          const float_t &year,
          const float_t &latitude, const float_t &elevation, const float_t &height_km){
        static const struct coef_t {
          float_t coef[3];
        } tbl_hyd_avg[] = {
          {1.2769934e-3, 2.9153695e-3, 62.610505e-3}, // 15
          {1.2683230e-3, 2.9152299e-3, 62.837393e-3}, // 30
          {1.2465397e-3, 2.9288445e-3, 63.721774e-3}, // 45
          {1.2196049e-3, 2.9022565e-3, 63.824265e-3}, // 60
          {1.2045996e-3, 2.9024912e-3, 64.258455e-3}, // 75
        }, tbl_hyd_amp[] = {
          {0.0,          0.0,          0.0         }, // 15
          {1.2709626e-5, 2.1414979e-5, 9.0128400e-5}, // 30
          {2.6523662e-5, 3.0160779e-5, 4.3497037e-5}, // 45
          {3.4000452e-5, 7.2562722e-5, 84.795348e-5}, // 60
          {4.1202191e-5, 11.723375e-5, 170.37206e-5}, // 75
        }, tbl_wet[] = {
          {5.8021897e-4, 1.4275268e-3, 4.3472961e-2}, // 15
          {5.6794847e-4, 1.5138625e-3, 4.6729510e-2}, // 30
          {5.8118019e-4, 1.4572752e-3, 4.3908931e-2}, // 45
          {5.9727542e-4, 1.5007428e-3, 4.4626982e-2}, // 60
          {6.1641693e-4, 1.7599082e-3, 5.4736038e-2}, // 75
        };
        static const int tbl_length(sizeof(tbl_hyd_avg) / sizeof(tbl_hyd_avg[0]));
        static const float_t delta(M_PI / 180 * 15);
        float_t idx_f(latitude / delta);
        int idx(idx_f);

        coef_t abc_avg, abc_amp, abc_wet;
        if(idx < 1){
          abc_avg = tbl_hyd_avg[0];
          abc_amp = tbl_hyd_amp[0];
          abc_wet = tbl_wet[0];
        }else if(idx >= (tbl_length - 1)){
          abc_avg = tbl_hyd_avg[tbl_length - 1];
          abc_amp = tbl_hyd_amp[tbl_length - 1];
          abc_wet = tbl_wet[tbl_length - 1];
        }else{
          // interpolation
          float_t weight_b((idx_f - idx) / delta), weight_a(1. - weight_b);
          for(int i(0); i < 3; ++i){
            abc_avg = tbl_hyd_avg[i] * weight_a + tbl_hyd_avg[i + 1] * weight_b;
            abc_amp = tbl_hyd_amp[i] * weight_a + tbl_hyd_amp[i + 1] * weight_b;
            abc_wet =     tbl_wet[i] * weight_a +     tbl_wet[i + 1] * weight_b;;
          }
        }

        NiellMappingFunction res;
        float_t sin_elv(sin(elevation));
        {
          float_t xi[3];
          float_t k_amp(cos(M_PI * 2 * (year - (28. / 365.25))));
          for(int i(0); i < 3; ++i){
            xi[i] = abc_avg.coef[i] - abc_amp.coef[i] * k_amp;
          }

          static const float_t abc_ht[] = {2.53e-5, 5.49e-3, 1.14e-3};
          res.hydrostatic = marini1972(sin_elv, xi)
              + ((1. / sin_elv) - marini1972(sin_elv, abc_ht)) * height_km;
        }
        res.wet = marini1972(sin_elv, abc_wet);
        return res;
      }
      NiellMappingFunction(
          const enu_t &relative_pos,
          const llh_t &usrllh,
          const gps_time_t &t){
        (*this) = get(t.year(), usrllh.latitude(), relative_pos.elevation(), usrllh.height() / 1E3);
      }
    };

    static float_t tropo_correction_zenith_hydrostatic_Saastamoinen(
        const float_t &latitude, const float_t &p_hpa, const float_t &height_km){
      return (0.0022767 * p_hpa)
          / (1. - 0.00266 * cos(latitude * 2) - 0.00028 * height_km);
    }

    /**
     * Calculate correction value in accordance with tropospheric model
     *
     * @param sat satellite position (absolute position, XYZ)
     * @param usr user position (absolute position, XYZ)
     * @return correction in meters
     */
    float_t tropo_correction(
        const xyz_t &sat,
        const xyz_t &usr) const {
      return tropo_correction(
          enu_t::relative(sat, usr),
          usr.llh());
    }

    struct TropospericCorrection {
      const GPS_SpaceNode<float_t> &space_node;
      float_t operator()(
          const enu_t &relative_pos,
          const llh_t &usrllh) const {
        return space_node.tropo_correction(relative_pos, usrllh);
      }
      float_t operator()(
          const xyz_t &sat,
          const xyz_t &usr) const {
        return space_node.tropo_correction(sat, usr);
      }
    };
    TropospericCorrection tropo_correction() const {
      TropospericCorrection res = {*this};
      return res;
    }
};

template <class FloatT>
const typename GPS_SpaceNode<FloatT>::float_t GPS_SpaceNode<FloatT>::light_speed = 2.99792458E8;

template <class FloatT>
const typename GPS_SpaceNode<FloatT>::float_t GPS_SpaceNode<FloatT>::L1_Frequency = 1575.42E6;

#define GPS_SC2RAD 3.1415926535898
template <class FloatT>
const typename GPS_SpaceNode<FloatT>::float_t GPS_SpaceNode<FloatT>::SC2RAD = GPS_SC2RAD;

template <class FloatT>
const typename GPS_SpaceNode<FloatT>::float_t GPS_SpaceNode<FloatT>::L2_Frequency = 1227.6E6;

template <class FloatT>
const typename GPS_SpaceNode<FloatT>::float_t GPS_SpaceNode<FloatT>::gamma_L1_L2 = (FloatT)(77 * 77) / (60 * 60);

#define POWER_2(n) \
(((n) >= 0) \
  ? (float_t)(1 << (n >= 0 ? n : 0)) \
  : (((float_t)1) / (1 << (-(n) >= 30 ? 30 : -(n > 0 ? 0 : n))) \
    / (1 << (-(n) >= 30 ? (-(n) - 30) : 0))))
template <class FloatT>
const typename GPS_SpaceNode<FloatT>::float_t GPS_SpaceNode<FloatT>::Ionospheric_UTC_Parameters::raw_t::sf[] = {
  POWER_2(-30), // alpha0
  POWER_2(-27), // alpha1
  POWER_2(-24), // alpha2
  POWER_2(-24), // alpha3
  POWER_2( 11), // beta0
  POWER_2( 14), // beta1
  POWER_2( 16), // beta2
  POWER_2( 16), // beta3
  POWER_2(-50), // A1
  POWER_2(-30), // A0
};

template <class FloatT>
const typename GPS_SpaceNode<FloatT>::float_t GPS_SpaceNode<FloatT>::SatelliteProperties::Ephemeris::raw_t::sf[] = {
  POWER_2(-31), // t_GD
  POWER_2(4),   // t_oc
  POWER_2(-31), // a_f0
  POWER_2(-43), // a_f1
  POWER_2(-55), // a_f2

  POWER_2(-5),                // c_rs
  GPS_SC2RAD * POWER_2(-43),  // delta_n
  GPS_SC2RAD * POWER_2(-31),  // M0
  POWER_2(-29),               // c_uc
  POWER_2(-33),               // e
  POWER_2(-29),               // c_us
  POWER_2(-19),               // sqrt_A
  POWER_2(4),                 // t_oe

  POWER_2(-29),               // c_ic
  GPS_SC2RAD * POWER_2(-31),  // Omega0
  POWER_2(-29),               // c_is
  GPS_SC2RAD * POWER_2(-31),  // i0
  POWER_2(-5),                // c_rc
  GPS_SC2RAD * POWER_2(-31),  // omega
  GPS_SC2RAD * POWER_2(-43),  // dot_Omega0
  GPS_SC2RAD * POWER_2(-43),  // dot_i0
};

template <class FloatT>
const typename GPS_SpaceNode<FloatT>::float_t GPS_SpaceNode<FloatT>::SatelliteProperties::Almanac::raw_t::sf[] = {
  POWER_2(-21),               // e
  POWER_2(12),                // t_oa
  GPS_SC2RAD * POWER_2(-19),  // delta_i
  GPS_SC2RAD * POWER_2(-38),  // dot_Omega0
  POWER_2(-11),               // sqrt_A
  GPS_SC2RAD * POWER_2(-23),  // Omega0
  GPS_SC2RAD * POWER_2(-23),  // omega
  GPS_SC2RAD * POWER_2(-23),  // M0
  POWER_2(-20),               // a_f0
  POWER_2(-38),               // a_f1
};
#undef POWER_2
#undef GPS_SC2RAD

template <class FloatT>
const typename GPS_SpaceNode<FloatT>::float_t GPS_SpaceNode<FloatT>::SatelliteProperties::Ephemeris::URA_limits[] = {
  2.40,
  3.40,
  4.85,
  6.85,
  9.65,
  13.65,
  24.00,
  48.00,
  96.00,
  192.00,
  384.00,
  768.00,
  1536.00,
  3072.00,
  6144.00,
};

template <class FloatT>
const int GPS_SpaceNode<FloatT>::SatelliteProperties::Ephemeris::URA_MAX_INDEX
    = sizeof(URA_limits) / sizeof(URA_limits[0]);

#ifdef POW2_ALREADY_DEFINED
#undef POW2_ALREADY_DEFINED
#else
#undef pow2
#endif
#ifdef POW3_ALREADY_DEFINED
#undef POW3_ALREADY_DEFINED
#else
#undef pow3
#endif

#endif /* __GPS_H__ */
