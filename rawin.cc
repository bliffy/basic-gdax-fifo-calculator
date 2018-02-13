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

#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <assert.h>
#include <map>
#include <string.h>
#include <algorithm>
#include <queue>

#include "enums.h"
#include "rawrec.h"
#include "filehelp.h"
#include "joinrec.h"

#define USE_STRING_TIMESTAMPS 1

#define ABSDIFF(x,y) ((x<y) ? (y-x) : (x-y))
// macro for roughly zero floating point value
#define NONZ(x) (x>0.0000001)
// second time tolerance for approximating value
#define SEC_TOLERANCE 3*60*60 // 3 hours

#define USAGE "Example Usage: ./runfifo my.csv 2017 > output.csv\n"

typedef std::vector<RawRec> RawRecVec_t;
typedef std::map<int,RawRecVec_t> RawRecJoiner_t;
typedef std::vector<JoinedRec> JoinedVec_t;
RawRecVec_t orig;
RawRecJoiner_t joiner;
JoinedVec_t joined;

int year;

int load_file(const char* fn);
void print_joined_recs(void);
int basic_fifo(void);

int main(int argc, char** argv){
  if (argc<2) {
    std::cerr<<"no csv file passed\n";
    std::cerr<<USAGE;
    return 1;
  }
  if (argc<3) {
    std::cerr<<"pass year to calculate\n";
    std::cerr<<USAGE;
    return 1;
  }

  if ( !load_file(argv[1]) ) {
    std::cerr<<"could not load file\n";
    return 1;
  }
  year = atoi(argv[2]);
  std::cerr<<"designated year: "<<year<<'\n';

  std::cerr<<"loaded "<<orig.size()<<" raw records\n\n";

  // group based on id
  for (int i=0;i<orig.size();i++) {
    RawRecJoiner_t::iterator f = joiner.find(orig[i].id);
    if (f==joiner.end()) {
      RawRecVec_t v;
      v.push_back(orig[i]);
      joiner.insert(std::make_pair<>(orig[i].id,v));
    } else {
      f->second.push_back(orig[i]);
    }
  }

  // verify all in each group have same timestamp
  RawRecJoiner_t::iterator j;
  for (j=joiner.begin();j!=joiner.end();j++) {
    if (j->first==0) continue;//skip deposit/withdraws
    int time0 = j->second[0].timeParsed;
    if (0==time0)
      assert(!"error:unparsed time");
    for (int n=1;n<j->second.size();n++){
      if (time0!=j->second[n].timeParsed)
        assert(!"error:mismatched times of same id");
    }
  }

  // convert groups into joined records
  for (j=joiner.begin();j!=joiner.end();j++) {
    joined.push_back(JoinedRec(j->second));
  }

  // sort on time
  std::sort(joined.begin(),joined.end());

  std::cerr<<"produced "<<joined.size()
            <<" joined records\n\n";

  // count how many non-USD-paired records we have
  std::vector<JoinedVec_t::iterator> toFix;
  for (JoinedVec_t::iterator jr=joined.begin();
       jr!=joined.end(); jr++)
  {
    if (jr->inUnit!=USD && jr->outUnit!=USD)
      toFix.push_back(jr);
  }
  std::cerr<<"records not incorporating USD: "
           <<toFix.size()<<'\n';

  if (toFix.size()>0)
    std::cerr<<"attempting to compensate\n";
  for (size_t i=0;i<toFix.size();i++) {
    // these tell us what unit we should look for and how
    // close in time our sample is to the real transaction
    JoinedVec_t::iterator t = toFix[i];
    long double rateA, rateB;
    unsigned int diffA=0xffffffff, diffB=0xffffffff, diff;
    int unitA=t->inUnit, unitB=t->outUnit;
    // scan for best matches
    JoinedVec_t::iterator s;
    for (s=joined.begin();s!=joined.end();s++) {
      if (!NONZ(s->amntIn) || !NONZ(s->amntOut))
        continue;
      diff = ABSDIFF(s->timeParsed,t->timeParsed);
      if (s->inUnit==USD) {
        if (s->outUnit==unitA && diff<diffA) {
          rateA = s->amntIn / s->amntOut;
          diffA = diff;
        } else if (s->outUnit==unitB && diff<diffB) {
          rateB = s->amntIn / s->amntOut;
          diffB = diff;
        }
      } else if (s->outUnit==USD) {
        if (s->inUnit==unitA && diff<diffA) {
          rateA = s->amntOut / s->amntIn;
          diffA = diff;
        } else if (s->inUnit==unitB && diff<diffB) {
          rateB = s->amntOut / s->amntIn;
          diffB = diff;
        }
      }
    }
    // did we get any samples we can use?
    if (diffA<diffB || (diffA==diffB && diffA!=0xffffffff)) {
      // A is the best we found
      if (diffA>SEC_TOLERANCE) {
        std::cerr<<"non-fiot trade with unguessable value\n";
        return 1;
      }
      std::cerr<<"sample time diff: "<<diffA<<" seconds\n";
      std::cerr<<"rate approx: "<<rateA
                <<" USD/"<<UNIT[t->inUnit]<<'\n';
      JoinedRec addMe(*t);
      // modify original to be a USD sell
      t->outUnit = USD;
      t->amntOut = t->amntIn * rateA;
      // convert fee if necessary
      if (t->feeUnit==unitA) {
        t->feeUnit = USD; t->amntFee *= rateA;
      } else if(t->feeUnit==unitB) {
        long double BtoA = addMe.amntIn / addMe.amntOut;
        t->feeUnit = USD; t->amntFee *= (BtoA * rateA);
      }
      // add new to encompass the buy
      addMe.inUnit = USD;
      addMe.amntIn = t->amntOut;
      addMe.timeParsed++; // give us a 1sec delay
      addMe.amntFee = 0.0;
      addMe.feeUnit = USD;
      joined.push_back(addMe);
    } else if (diffB<diffA) {
      // B is the best we found
      if (diffB>SEC_TOLERANCE) {
        std::cerr<<"non-fiot trade with unguessable value\n";
        return 1;
      }
      std::cerr<<"sample time diff: "<<diffB<<" seconds\n";
      std::cerr<<"rate approx: "<<rateB
               <<" USD/"<<UNIT[t->outUnit]<<'\n';
      JoinedRec addMe(*t);
      // modify original to be a USD sell
      t->outUnit = USD;
      t->amntOut = t->amntOut * rateB;
      // convert fee if necessary
      if (t->feeUnit==unitA) {
        long double AtoB = t->amntOut / t->amntIn;
        t->feeUnit = USD; t->amntFee *= (AtoB * rateB);
      } else if (t->feeUnit==unitB) {
        t->feeUnit = USD; t->amntFee *= rateB;
      }
      // add new to encompass the buy
      addMe.inUnit = USD;
      addMe.amntIn = t->amntOut;
      addMe.timeParsed++;//give us a 1sec delay
      addMe.amntFee = 0.0;
      addMe.feeUnit = USD;
      joined.push_back(addMe);
    }

    std::cerr<<"    trade: "<<t->amntIn<<' '
             <<UNIT[t->inUnit]<<" for "
             <<t->amntOut<<' '<<UNIT[t->outUnit]<<'\n';
    JoinedRec& x=joined.back();
    std::cerr<<"    trade: "<<x.amntIn<<' '
             <<UNIT[x.inUnit]<<" for "
             <<x.amntOut<<' '<<UNIT[x.outUnit]<<'\n';

  }
  if (toFix.size()>0) {
    // shuffle our new records into correct position
    std::sort(joined.begin(),joined.end());
    std::cerr<<"\nnew joined record count: "
             <<joined.size()<<'\n';
  }
  std::cerr<<'\n';

  // print_joined_recs();

  // try to do fifo
  if (! basic_fifo() )
    return 1;

  return 0;
}

void print_joined_recs(void) {
  // roughly print out the joined records
  std::cout.setf(std::ios::fixed);
  std::cout<<'\n';
  for (JoinedVec_t::iterator jr=joined.begin();
       jr!=joined.end(); jr++)
  {
    const char * in = UNIT[jr->inUnit]; if (!in) in = "";
    const char * out = UNIT[jr->outUnit];if (!out) out = "";
    const char * fee = UNIT[jr->feeUnit];if (!fee) fee = "";
    std::cout<<jr->timeParsed<<','
             <<jr->amntIn<<' '<<in <<','
             <<jr->amntOut<<' '<< out <<','
             <<jr->amntFee<<' '<< fee <<'\n';
  }
  std::cout<<'\n';
}

int load_file(const char* fn) {
  std::ifstream file;
  file.open(fn);
  if (file.fail()) return 0;
  char buffer[1024];
  while(readline(file,buffer,"type")) {
    RawRec rr;
    if (!rr.parseLine(buffer)) {
      file.close();
      return 0;//fail
    }
    orig.push_back(rr);
  }
  file.close();
  return 1;
}

typedef struct _allotment_t {
  long double volume;
  long double price;
  time_t time;
} allotment_t;
//-
int basic_fifo(void) {
  typedef std::queue<allotment_t> holdings_t;
  holdings_t holdings[5];//use UNIT as index
  int discrepancies = 0;
  long double rate, total, fees=0.0;
  int unit;
  time_t now;

  // for conversion on-the-fly
  struct tm then_tm;
  struct tm now_tm;
  char thenBuf[255];
  char nowBuf[255];

  // csv header
  std::cout.setf(std::ios::fixed);
  std::cout<<"buy_date,sell_date,days_apart,item,gain,loss\n";

  // iterate through our joined records chronologically
  for (size_t i=0;i<joined.size();i++) {

    // find out the year of the record
    struct tm *gmt = gmtime(&joined[i].timeParsed);
    bool shouldPrint = (year==(gmt->tm_year+1900));

    // accumulate fee
    assert(joined[i].feeUnit==USD || joined[i].feeUnit==0);
    if (joined[i].feeUnit==USD && shouldPrint)
      fees += joined[i].amntFee;

    if (joined[i].inUnit==USD) {
      // record represents a buy
      unit = joined[i].outUnit;
      allotment_t a;
      a.volume = joined[i].amntOut;
      assert(NONZ(joined[i].amntOut));
      a.price = joined[i].amntIn / joined[i].amntOut;
      a.time = joined[i].timeParsed;
      // add to available holdings
      holdings[unit].push(a);
    } else { // outUnit==USD
      // record represents a sell
      assert(joined[i].outUnit==USD);
      unit   = joined[i].inUnit;
      rate   = joined[i].amntOut / joined[i].amntIn;
      total  = joined[i].amntIn;
      now    = joined[i].timeParsed;
      while ( NONZ(total) && !holdings[unit].empty() ) {
        long double avail = holdings[unit].front().volume;
        long double price = holdings[unit].front().price;
        time_t then = holdings[unit].front().time;
        // if avail will be depleted
        if (avail <= total) {
          total -= avail;

          if (shouldPrint) {
#if ( USE_STRING_TIMESTAMPS==1 )
            gmtime_r(&then,&then_tm);
            gmtime_r(&now,&now_tm);
            time2str(&then_tm,thenBuf,255);
            time2str(&now_tm,nowBuf,255);
            // timestamps:
            std::cout<<thenBuf<<','<<nowBuf<<','
                     <<UNIT[unit]<<',';
#else
            std::cout<<then<<','<<now<<','
                     <<UNIT[unit]<<',';
#endif
            // days apart:
            std::cout<<((now-then)/(60*60*24))<<',';

            if (price /*old*/ < rate /*new*/) // gain
              std::cout<<(avail*(rate-price))<<','<<0.0<<'\n';
            else // loss
              std::cout<<0.0<<','<<(avail*(price-rate))<<'\n';
          }

          holdings[unit].pop();
        } else {//some will remain
          holdings[unit].front().volume -= total;

          if (shouldPrint) {
#if (USE_STRING_TIMESTAMPS==1 )
            gmtime_r(&then,&then_tm);
            gmtime_r(&now,&now_tm);
            time2str(&then_tm,thenBuf,255);
            time2str(&now_tm,nowBuf,255);
            // timestamps:
            std::cout<<thenBuf<<','<<nowBuf<<','
                     <<UNIT[unit]<<',';
#else
            std::cout<<then<<','<<now<<','
                     <<UNIT[unit]<<',';
#endif
            // days apart:
            std::cout<<((now-then)/(60*60*24))<<',';
            if (price /*old*/ < rate /*new*/) // gain
              std::cout<<(total*(rate-price))<<','<<0.0<<'\n';
            else
              std::cout<<0.0<<','<<(total*(price-rate))<<'\n';
          }

          total = 0.0;
        }
      }

      // were we able to fulfil our sale?
      if ( NONZ(total) ) {
        std::cerr<<"error: unfillable sale of "
                 <<total<<UNIT[unit]<<"\n";
        discrepancies++;
      }
    }
  }//end record iteration

  std::cerr<<"\ndiscrepancy count: "<<discrepancies
           <<"\n\n";
  std::cerr<<"total fees: "<<fees<<" USD\n";
  return 1;
}

