/*
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef __FIN_JOINREC_H__
#define __FIN_JOINREC_H__

#include <sstream>
#include <string>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <vector>

#include "enums.h"
#include "rawrec.h"

class JoinedRec {
public:
  int id;
  time_t timeParsed;
  long double amntIn;
  long double amntOut;
  long double amntFee;
  int inUnit;
  int outUnit;
  int feeUnit;

  JoinedRec(void)
   : id(0),
     timeParsed(0),
     amntIn(0.0), amntOut(0.0), amntFee(0.0),
     inUnit(USD), outUnit(USD), feeUnit(0) { }
  JoinedRec(const JoinedRec& r)
   : id(r.id),
     timeParsed(r.timeParsed),
     amntIn(r.amntIn),
     amntOut(r.amntOut),
     amntFee(r.amntFee),
     inUnit(r.inUnit),
     outUnit(r.outUnit),feeUnit(r.feeUnit)
     { }
  JoinedRec(const std::vector<RawRec>& recs)
   : id(0),
     timeParsed(0),
     amntIn(0.0), amntOut(0.0), amntFee(0.0),
     inUnit(USD), outUnit(USD), feeUnit(0)
  {
    std::vector<RawRec>::const_iterator r;
    id = recs[0].id;
    timeParsed = recs[0].timeParsed;
    for (r=recs.begin();r!=recs.end();r++){
      switch (r->action) {
      case WITHDRAWAL:
        amntOut = - r->amount;
        outUnit = r->unit;
        break;
      case DEPOSIT:
        amntIn = r->amount;
        inUnit = r->unit;
        break;
      case FEE:
        amntFee = - r->amount;
        feeUnit = r->unit;
        break;
      default: // match
        if (r->amount >= 0.0) {
          amntOut = r->amount;
          outUnit = r->unit;
        } else {
          amntIn = - r->amount;
          inUnit = r->unit;
        }
        break;
      }//end switch
    }//end for
    // try to automatically convert fee
    if (feeUnit!=USD && amntFee>0.000001) {
      if (feeUnit==inUnit && outUnit==USD
          && amntOut>0.000001) {
        feeUnit = USD;
        amntFee *= amntIn/amntOut;
      } else if (feeUnit==outUnit && inUnit==USD
                 && amntIn>0.000001) {
        feeUnit = USD;
        amntFee *= amntOut/amntIn;
      }
    }
  }//end vec construct
  inline bool operator< (const JoinedRec& o) const {
    return this->timeParsed < o.timeParsed;
  }
};

#endif

