// -*- C++ -*-
#ifndef HF_SCLXFORM_H__
#define HF_SCLXFORM_H__

#include <math.h>
#include "hf-hl.h"

// can't have pure virtual functions in scalar_op because they need to be
// registered with lua.
struct scalar_op1 {
	virtual const char *_type() const {
		return  "scalar_op1";
	}
	virtual PTYPE operator()(PTYPE) const {
		return 0;
	}
};

struct scalar_op2 {
	virtual const char *_type() const {
		return "scalar_op2";
	}
	virtual PTYPE operator()(PTYPE, PTYPE) const {
		return 0;
	}
};

hfield *h_op1(hfield *h1, const scalar_op1 &op);
hfield *h_op2(hfield *h1, hfield *h2, int xo, int yo, const scalar_op2 &op);
#define STR_(x) #x

// macro form: DECLAREop1_0 means: op1 (single image operation), _0 (0 extra
// parameters in constructor)
// OP1
#define DECLAREop1_0(sname) struct op1_##sname : public scalar_op1 {\
  const char *_type() const { return STR_(op1_##sname); }\
  PTYPE operator()(PTYPE p) const {
#define END } };

// TODO: hyperbolic functions
DECLAREop1_0(sin)	return sin(p); END
DECLAREop1_0(asin)	if(p < -1) p = -1; if(p > 1) p = 1; return asin(p); END
DECLAREop1_0(cos)	return cos(p); END
DECLAREop1_0(acos)	if(p < -1) p = -1; if(p > 1) p = 1; return acos(p); END
DECLAREop1_0(tan)	return cos(p) == 0 ? 0 : tan(p); END
DECLAREop1_0(atan)	return atan(p); END
DECLAREop1_0(abs)	return fabs(p); END
DECLAREop1_0(inv)	return p == 0 ? 0 : 1 / p; END
DECLAREop1_0(log)	return p < 0 ? 1 : log(p); END

#define DECLAREop1_1(sname) struct op1_##sname : public scalar_op1 {\
  D fac_;\
  op1_##sname(D fac) : fac_(fac) { }\
  const char *_type() const { return STR_(op1_##sname); }\
  PTYPE operator()(PTYPE p) const {

DECLAREop1_1(pow1)	return SGN(p)*pow(fabs(p), fac_); END
DECLAREop1_1(disc)	return (int)((p*fac_-0.000001)/fac_); END
DECLAREop1_1(mod)	return p-SGN(p)*fabs(((int)(p/fac_))*fac_); END

#define DECLAREop1_1a(sname, op) DECLAREop1_1(sname) return p op fac_; END

DECLAREop1_1a(add1, +)
DECLAREop1_1a(sub1, -)
DECLAREop1_1a(mul1, *)
DECLAREop1_1a(div1, /)

struct scalar_op1_fc : public scalar_op1 {
	D fac_, fac2_, sf2_, sf3_;
	scalar_op1_fc(D fac, D fac2, D min, D max) : fac_(fac), fac2_(fac2) {
		sf2_ = max - fac; if(sf2_ != 0) sf2_ = fac2 / sf2_;
		sf3_ = fac - min; if(sf3_ != 0) sf3_ = fac2 / sf3_;
	}
};
	
struct op1_floor : public scalar_op1_fc {
	op1_floor(D fac, D fac2, D min, D max) :
		scalar_op1_fc(fac, fac2, min, max) { }
	const char *_type() const { return "op1_floor"; }
	PTYPE operator()(PTYPE p) const {
		return (p < fac_) ? ((fac2_ > 0) ? fac_ + sf3_*(1-1/(1+p-fac_)) : fac_) : p;
	}
};

struct op1_ceil : public scalar_op1_fc {
	op1_ceil(D fac, D fac2, D min, D max) :
		scalar_op1_fc(fac, fac2, min, max) { }
	const char *_type() const { return "op1_ceil"; }
	PTYPE operator()(PTYPE p) const {
		return (p > fac_) ? ((fac2_ > 0) ? fac_ + sf2_*(1-1/(1+p-fac_)) : fac_) : p;
	}
};

// OP2
#define DECLAREop2_0(sname) struct op2_##sname : public scalar_op2 {\
  const char *_type() const { return STR_(op1_##sname); }\
  PTYPE operator()(PTYPE p1, PTYPE p2) const {
#define DECLAREop2a_0(sname, op) DECLAREop2_0(sname) return p1 op p2; END

DECLAREop2a_0(add2, +)
DECLAREop2a_0(sub2, -)
DECLAREop2a_0(mul2, *)
DECLAREop2a_0(div2, /)

DECLAREop2_0(rmag) return hypot(p1, p2); END
DECLAREop2_0(pow2) return pow(p1 < 0 ? 0 : p1, p2 < 0 ? 0 : p2); END
DECLAREop2_0(com) return MAX(p1, p2); END
DECLAREop2_0(comn) return MIN(p1, p2); END

#endif
