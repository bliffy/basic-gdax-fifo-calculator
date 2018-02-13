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

#ifndef __FIN_RAWREC_H__
#define __FIN_RAWREC_H__

#include <sstream>
#include <string>
#include <assert.h>
#include <string.h>
#include <time.h>

#include "enums.h"
#include "timeparse.h"

class RawRec {
public:          // field position
  std::string type;    // 1
  std::string time;    // 2
  long double amount;  // 3
  long double balance; // 4
  int unit;            // 5  enumerated
  // order id          // 6
  int id;              // 7
  // trans id          // 8

  int action; // enumerated
  time_t timeParsed;

  RawRec(void)
    : amount(0.0),
      balance(0.0),
      unit(0),
      id(0),
      action(0),
      timeParsed(0) { }
  RawRec(const RawRec& r)
   : type(r.type),
     time(r.time),
     amount(r.amount),
     balance(r.balance),
     unit(r.unit),
     id(r.id),
     action(r.action),
     timeParsed(r.timeParsed) { }

  // parse populate fields from line string
  inline int parseLine(char* line) {
    // step break into fields
    int i=0;           // iterator
    char* mark=line;   // start of last field
    int field=1;       // field index
    // reset old values
    while (line[i] != 0) {
      // separator
      if (line[i]==',' || line[i]=='\n') {
        line[i]=0; // terminate piece
        std::stringstream convert(mark);
        if (convert.str().size()==0)
          convert.str("0");
        //assign marked section to field number
        switch(field){
        case 1: this->type=convert.str(); break;
        case 2: this->time=convert.str(); break;
        case 3: convert>> this->amount; break;
        case 4: convert>> this->balance; break;
        case 5: this->unit=strToUnit(convert.str()); break;
        case 6: /* ignored */ break;
        case 7: convert>> this->id; break;
        case 8: /* ignored */ break;
        default: assert(!"field error"); return 0;
        }
        field++;//next field
        mark=line+(1+i);//start at next position
      }//end if separator
      i++;
    }//end tokenizing loop
    this->action = strToAction(
      this->type,
      this->amount);
    this->timeParsed = timeparse_str2time(
      this->time.c_str(), NULL);
    return 1;
  }// end parseLine()

  inline int strToUnit(const std::string& str) {
    if (str=="BTC") return BTC;
    else if (str=="BCH") return BCH;
    else if (str=="LTC") return LTC;
    else if (str=="ETH") return ETH;
    else if (str=="USD") return USD;
    else return -1;
  }
  inline int strToAction(const std::string& str,
                         long double amnt) {
    assert(amnt<0.0 || amnt>0.0);
    if (str=="match" && amnt>0.0) return BUY;
    else if (str=="match" && amnt<0.0) return SELL;
    else if (str=="fee") return FEE;
    else if (str=="deposit") return DEPOSIT;
    else if (str=="withdrawal") return WITHDRAWAL;
    assert(0);
    return -1;
  }
};

#endif

