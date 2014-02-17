/* 
 * File:   PRcodes.cpp
 * Author: Jarek Duda
 *
 * Created on November 21, 2013, 10:46 AM
 */

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <math.h> 
#include "bitmap_image.hpp"
typedef unsigned char byte;
using namespace std;

const short N=8;                // we use 8 bit blocks
short F=2;                      // number of freedom bits in block, rate is 1-F/N
const uint64_t mxactive=10000, mxhistory=100000000;       // maximum number of types of nodes
const uint states_count = 1<<N;
const uint mask=states_count-1; 
int seed=1;                     // can be cryptographic key
uint64_t cstate, nstate;        // current state and new state
uint64_t *tr = new uint64_t[states_count];      //transition function
uint *rev = new uint[states_count];             //reversed bijection from transition function for decoding step
int64_t i,j,k;
byte x,y;                       // symbol before and after encoding

struct history{                 // history to recreate all codes
    uint64_t fath;              // position of the previous node
    byte y;                     // encoded byte
};
struct active{
    uint64_t fath;              // father's position in history
    byte y;                     // encoded byte
    uint64_t state;             // current state
    float wgh;                  // current weight
};

void generate_tr(){        //generates transition function 'tr' and its reverse for the first byt for decoding 'rev'
  uint temp[states_count];
  uint k;
  uint64_t m=1;
  srand(seed);
  for(uint i=0;i<(64/N);i++){                              //choose permutation for position m
        for (int32_t j=0 ; j < states_count ; j++) temp[j] = j;
        for (int32_t j=0 ; j < states_count ; j++) {
                k = rand()%(states_count-j); tr[j] +=  m*temp[k];  
                if(m==1) rev[temp[k]]=j;
                temp[k]=temp[states_count-j-1];
        };
    m*=states_count;
  }   
}

inline void enc()                                      //single step of encoding: x from cstate, getting y and nstate
{   nstate=cstate ^ tr[x];
    y=nstate & mask;
    nstate=nstate >> N | nstate << (64-N);
}
inline void dec()                                      //single step of decoding: x from cstate, getting y and nstate
{   x=rev[(y^cstate)&mask];
    nstate=cstate^tr[x];
    nstate=nstate >> N | nstate << (64-N);
}

void encode(){                                  // procedure searching for the best encoding                               
   float wghts[2][8], wghs[256];                // weights for current step
   float mxwg=1, mnwg, tmpr;                    // mxwg is maximum weight to be processed to the next step
   byte encs;                                   // oldest bits of the current byte to encode - youngest F filled in all ways
   uint32_t step=0,msgp=0,msgbp=0;
   history *hist=new history[mxhistory];          // storing tree of all encodings (for final retrieval)
   uint64_t chist=0;
   active *cur=new active[mxactive], *prev=new active[mxactive], *tact, nact;    // active nodes for current and previous step
   uint64_t nprev=1, ncur=0;                      // number of previous and current codes
   uint32_t bucket[1000];                         // bucket to find given number of the smallest weights
   
    bitmap_image image("picture.bmp");             //load probabilities
    unsigned int xp=50, yp=50;
    unsigned char red, green,blue;
    const unsigned int height = image.height(), width  = image.width();
    float *prob = new float [width*height+10];          // sequence of Pr(1)
    for(i=0;i<width*height;i++)                         // load probabilities
    {if((i%width)==0) {xp+=1;yp+=0;};                   // to visit all pixels
     xp=(xp+19)%width; yp=(yp+29)%height;               
     image.get_pixel(xp,yp,red,green,blue);
     prob[i]=(((float)red+(float)green+(float)blue)/3/256 + 0.0001)*0.9998;   // rescale to prevent log(0)
    }
    
    uint bytenumber=ceil(width*height/8); 
    byte *xseq=new byte[bytenumber], *yseq=new byte[bytenumber];  // message before and after encoding
    char *msg=new char[bytenumber];                //the message to encode    
    ifstream data;
    data.open("data.dat");
    data.read(msg,bytenumber);
   
    prev->state=0; prev->wgh=0;                    // initial state and weight
   
   for(step=0;step<bytenumber;step++)              // THE ENCODING LOOP
   { 
     for (i=0;i<8;i++)   {wghts[0][i]=-log(1-prob[8*step+i]); wghts[1][i]=-log(prob[8*step+i]);};
     for (i=0;i<256;i++)                          // find weights for all possible output bytes ("wghs")
     {tmpr=0; 
       for(j=0;j<8;j++)  
         if((i&(1<<j))==0) tmpr+=wghts[0][j]; else tmpr+=wghts[1][j]; wghs[i]=tmpr;
     }
   
     encs=0;                                      // extract current bits to encode 
     for(i=F;i<8;i++)
        { encs += ((msg[msgp]&(1<<msgbp))>>msgbp)<<i;
          msgbp++;if(msgbp==8) {msgbp=0; msgp++;};
        }
     
     for (i=0;i<nprev;i++)                        // THE MAIN LOOP - expand prev[i] in all possible ways
         if(prev[i].wgh<=mxwg){                   // expand only nodes below mxwg
             hist[chist].fath=prev[i].fath;       // insert father to the history
             hist[chist].y=prev[i].y;
             cstate=prev[i].state;                // take current state
             tmpr=prev[i].wgh;                    // take current weight
             for(j=0;j<(1<<F);j++){               // all possible values of the freedom bits
               x=encs + j;                        // x is the message bits (enc) and freedom bits (i)
               enc();                             // encode step from cstate and x, getting nstate and y
               nact.fath=chist;                   // create new node 
               nact.state=nstate;
               nact.wgh=tmpr+wghs[y];
               nact.y=y;
               cur[ncur]=nact; ncur++;             // insert this new node to cur       
             }
             chist++;
         }
   
     tact = prev;  prev=cur; cur=tact;            // current nodes become the previous ones
     nprev=ncur; ncur=0;                          
   
     mxwg=mnwg=prev[0].wgh;                                   // FINDING MXWG FOR THE NEXT STEP
     for(i=1;i<nprev;i++)                                     // find maximum and minimum weight
        {tmpr=prev[i].wgh; mxwg=max(mxwg,tmpr); mnwg=min(mnwg,tmpr);}  
     if(nprev>(mxactive>>F)){
       tmpr=1000/(mxwg-mnwg); for(i=0;i<1000;i++) bucket[i]=0;
       for(i=0;i<nprev;i++) 
           bucket[(int) (tmpr*(prev[i].wgh-mnwg))]++;              
       tmpr=0;
       for(i=0;(i<1000)&&(tmpr < (mxactive>>F));i++) tmpr+=bucket[i];
       mxwg=mnwg+(i-2)*(mxwg-mnwg)/1000;                             // there is at most mxactive>>F below this mxwg
     }
   }                                                                 // end of encoding loop
   
   for(i=0;prev[i].wgh!=mnwg;i++);                                   // getting encoded sequence from the tree
   yseq[bytenumber-1]=prev[i].y; i=prev[i].fath;
   for(j=bytenumber-2;j>=0;j--)
   {yseq[j]=hist[i].y;i=hist[i].fath;}
   
   j=0;k=0;xp=50;yp=50;                                               // writing it in the bitmap 
    for(i=0;i<width*height;i++)
    {if((i%width)==0) {xp+=1;yp+=0;};
     xp=(xp+19)%width; yp=(yp+29)%height;
     if((yseq[j]&(1<<k))>0) {red=green=blue=255;} else {red=green=blue=0;}; 
     k++; if(k==8) {k=0;j++;};
     image.set_pixel(xp,yp,red,green,blue);    
    }
    image.save_image("code.bmp");
        
 delete [] hist,cur,prev,xseq,yseq,prob;
}

void decode()                                  
{   bitmap_image image("code.bmp");             //load probabilities
    unsigned int xp=50, yp=50;
    unsigned char red, green,blue;
    const unsigned int height = image.height();
    const unsigned int width  = image.width();
    uint msgp,msgbp;
    uint bytenumber=ceil(width*height/8); 
    char *msg=new char[bytenumber];                //the message to encode  
    
    y=0; msgp=0; msgbp=0; cstate=0; k=0; msg[0]=0;
    for(i=0;i<width*height;i++)
    {if((i%width)==0) {xp+=1;yp+=0;};
     xp=(xp+19)%width; yp=(yp+29)%height;
     image.get_pixel(xp,yp,red,green,blue);
     if(red+green+blue >= 3*128) y = y|(1<<k);
     k++;
     if(k==8)
        {dec();cstate=nstate; k=0; y=0;
                for(j=F;j<8;j++){
                        msg[msgp]= msg[msgp] | ((x>>j)&1)<<msgbp;
                        msgbp++;
                        if(msgbp==8) {msgbp=0; msgp++; msg[msgp]=0;}
                }
      }
   }
    
   ofstream out;
   out.open("out.dat");
   out.write(msg,msgp+1);
   out.close();   
   delete[] msg;
}


int main () {
    generate_tr();                                      // generate transition function for encoding/decoding
        
     encode();                                          // comment one
     decode();                                          // comment one
   
    delete[] tr, rev;
    return 0;
}
