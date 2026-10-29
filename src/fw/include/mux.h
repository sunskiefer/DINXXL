

void mux(uint8_t data){

digitalWrite(DATA_0_PIN,bitRead(data,0));
digitalWrite(DATA_1_PIN,bitRead(data,1));
digitalWrite(DATA_2_PIN,bitRead(data,2));
digitalWrite(DATA_3_PIN,bitRead(data,3));

}


void prepareFaders(){
  //match current positions to faderLast
  //so kit is unchanged after load
for(int16_t i=0;i<16;i++){

muxIndex=i;

mux(muxIndex);

int16_t input=analogRead(MUX1_PIN)>>5;
faderLast[muxIndex]=input;

input=analogRead(MUX2_PIN)>>5;
faderLast[muxIndex+16]=input;

input=analogRead(MUX3_PIN)>>5;
faderLast[muxIndex+32]=input;

if(muxIndex<8){
input=analogRead(MUX6_PIN)>>5;
faderLast[muxIndex+48]=input;
}

// buttons

bool butt=digitalRead(MUX4_PIN);
buttLast[muxIndex]=butt;

   butt=digitalRead(MUX5_PIN);
buttLast[muxIndex+16]=butt;



}
}


void readMux(){

int16_t input;
bool butt;

muxIndex++;
if(muxIndex>15)muxIndex=0;
mux(muxIndex);
if(mode<4){
input=analogRead(MUX1_PIN)>>5;
if((input>faderLast[muxIndex]+9)||(input<faderLast[muxIndex]-9)){movedFader(muxIndex,input>>3);faderLast[muxIndex]=input;}

mux(muxIndex);
input=analogRead(MUX2_PIN)>>5;
if((input>faderLast[muxIndex+16]+9)||(input<faderLast[muxIndex+16]-9)){movedFader(muxIndex+16,input>>3);faderLast[muxIndex+16]=input;}

mux(muxIndex);
input=analogRead(MUX3_PIN)>>5;
if((input>faderLast[muxIndex+32]+9)||(input<faderLast[muxIndex+32]-9)){movedFader(muxIndex+32,input>>3);faderLast[muxIndex+32]=input;}

if(muxIndex<8){
  mux(muxIndex);
input=analogRead(MUX6_PIN)>>5;
if((input>faderLast[muxIndex+48]+9)||(input<faderLast[muxIndex+48]-9)){movedFader(muxIndex+48,input>>3);faderLast[muxIndex+48]=input;}
}}

// buttons
mux(muxIndex);
   butt=digitalRead(MUX4_PIN);

if(butt!=buttLast[muxIndex]){buttLast[muxIndex]=butt;buttPressed(muxIndex,butt);}//8-15

mux(muxIndex);
   butt=digitalRead(MUX5_PIN);
if(butt!=buttLast[muxIndex+16]){buttLast[muxIndex+16]=butt;buttPressed(muxIndex+16,butt);}//8-15



}
