#include "pass1.cpp"

using namespace std;

ifstream intermediateFile;
ofstream errorFile,objectFile,ListingFile;


ofstream printtab ;
string writestring ;


bool isComment;
string label,opcode,operand,comment;
string operand1,operand2;

int lineNumber,blockNumber,address,startAddress;
string objectCode, writeData, currentRecord, modifi_rec="M^", endRecord, write_R_Data, write_D_Data,currentSectName="DEFAULT";
int sectionCounter=0;
int program_section_length=0;

int program_counter, base_register_value, currentTextRecordLength;
bool nobase;

string readTillTab(string data,int& index){
  string tempBuffer = "";

  while(index<data.length() && data[index] != '\t'){
    tempBuffer += data[index];
    index++;
  }
  index++;
  if(tempBuffer==" "){
    tempBuffer="-1" ;
  }
  return tempBuffer;
}
bool readIntermediateFile(ifstream& readFile,bool& isComment, int& lineNumber, int& address, int& blockNumber, string& label, string& opcode, string& operand, string& comment){
  string fileLine="";
  string tempBuffer="";
  bool tempStatus;
  int index=0;
  if(!getline(readFile, fileLine)){
    return false;
  }
  lineNumber = stoi(readTillTab(fileLine,index));

  isComment = (fileLine[index]=='.')?true:false;
  if(isComment){
    read_first_nonwhitespace(fileLine,index,tempStatus,comment,true);
    return true;
  }
  address = stringHexToInt(readTillTab(fileLine,index));
  tempBuffer = readTillTab(fileLine,index);
  if(tempBuffer == " "){
    blockNumber = -1;
  }
  else{
    blockNumber = stoi(tempBuffer);
  }
  label = readTillTab(fileLine,index);
  if(label=="-1"){
    label=" " ;
  }
  opcode = readTillTab(fileLine,index);
  if(opcode=="BYTE"){
    read_byte_operand(fileLine,index,tempStatus,operand);
  }
  else{
    operand = readTillTab(fileLine,index);
    if(operand=="-1"){
      operand=" " ;
    }
    if(opcode=="CSECT"){
      return true ;
    }
  }
  read_first_nonwhitespace(fileLine,index,tempStatus,comment,true);  
  return true;
  
}

string createObjectCodeFormat34(){
  string object_code;
  int halfBytes;
  halfBytes = (getFlagFormat(opcode)=='+')?5:3;

  if(getFlagFormat(operand)=='#'){//Immediate
    if(operand.substr(operand.length()-2,2)==",X"){//Error handling for Immediate with index based
      writeData = "Line: "+to_string(lineNumber)+" Index based addressing not supported with Indirect addressing";
      writeToFile(errorFile,writeData);
      object_code = intToHexString(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+1,2);
      object_code += (halfBytes==5)?"100000":"0000";
      return object_code;
    }

    string tempOperand = operand.substr(1,operand.length()-1);
    if(if_all_num(tempOperand)||((SYMTAB[tempOperand].exists=='y')&&(SYMTAB[tempOperand].relative==0))){//Immediate integer value
      int immediateValue;

      if(if_all_num(tempOperand)){
        immediateValue = stoi(tempOperand);
      }
      else{
        immediateValue = stringHexToInt(SYMTAB[tempOperand].address);
      }
      /*Process Immediate value*/
      if(immediateValue>=(1<<4*halfBytes)){//Can't fit it
        writeData = "Line: "+to_string(lineNumber)+" Immediate value exceeds format limit";
        writeToFile(errorFile,writeData);
        object_code = intToHexString(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+1,2);
        object_code += (halfBytes==5)?"100000":"0000";
      }
      else{
        object_code = intToHexString(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+1,2);
        object_code += (halfBytes==5)?'1':'0';
        object_code += intToHexString(immediateValue,halfBytes);
      }
      return object_code;
    }
    else if(SYMTAB[tempOperand].exists=='n') {
     
      if(halfBytes==3) { 
      writeData = "Line "+to_string(lineNumber);
     if(halfBytes==3){
         writeData+= " : Invalid format for external reference " + tempOperand; 
      } else{ 
         writeData += " : Symbol doesn't exists. Found " + tempOperand;
       } 
      writeToFile(errorFile,writeData);
      object_code = intToHexString(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+1,2);
      object_code += (halfBytes==5)?"100000":"0000";
      return object_code;
    }

    }
    else{
      int operandAddress = stringHexToInt(SYMTAB[tempOperand].address) + stringHexToInt(BLOCKS[BLocksNumToName[SYMTAB[tempOperand].blockNumber]].staAdr);

      /*Process Immediate symbol value*/
      if(halfBytes==5){/*If format 4*/
        object_code = intToHexString(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+1,2);
        object_code += '1';
        object_code += intToHexString(operandAddress,halfBytes);

        /*add modifacation record here*/
        modifi_rec += "M^" + intToHexString(address+1,6) + '^';
        modifi_rec += (halfBytes==5)?"05":"03";
        modifi_rec += '\n';

        return object_code;
      }

      /*Handle format 3*/
      program_counter = address + stringHexToInt(BLOCKS[BLocksNumToName[blockNumber]].staAdr);
      program_counter += (halfBytes==5)?4:3;
      int relAdr = operandAddress - program_counter;

      /*Try PC based*/
      if(relAdr>=(-2048) && relAdr<=2047){
        object_code = intToHexString(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+1,2);
        object_code += '2';
        object_code += intToHexString(relAdr,halfBytes);
        return object_code;
      }

      /*Try base based*/
      if(!nobase){
        relAdr = operandAddress - base_register_value;
        if(relAdr>=0 && relAdr<=4095){
          object_code = intToHexString(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+1,2);
          object_code += '4';
          object_code += intToHexString(relAdr,halfBytes);
          return object_code;
        }
      }

      /*Use direct addressing with no PC or base*/
      if(operandAddress<=4095){
        object_code = intToHexString(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+1,2);
        object_code += '0';
        object_code += intToHexString(operandAddress,halfBytes);

        /*add modifacation record here*/
        modifi_rec += "M^" + intToHexString(address+1+stringHexToInt(BLOCKS[BLocksNumToName[blockNumber]].staAdr),6) + '^';
        modifi_rec += (halfBytes==5)?"05":"03";
        modifi_rec += '\n';

        return object_code;
      }
    }
  }
  else if(getFlagFormat(operand)=='@'){
    string tempOperand = operand.substr(1,operand.length()-1);
    if(tempOperand.substr(tempOperand.length()-2,2)==",X" || SYMTAB[tempOperand].exists=='n'){//Error handling for Indirect with index based
      if(halfBytes==3) {
      writeData = "Line "+to_string(lineNumber);
      writeData += " : Symbol doesn't exists.Index based addressing not supported with Indirect addressing ";
      writeToFile(errorFile,writeData);
      object_code = intToHexString(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+2,2);
      object_code += (halfBytes==5)?"100000":"0000";
      return object_code;
    }

}
    int operandAddress = stringHexToInt(SYMTAB[tempOperand].address) + stringHexToInt(BLOCKS[BLocksNumToName[SYMTAB[tempOperand].blockNumber]].staAdr);
    program_counter = address + stringHexToInt(BLOCKS[BLocksNumToName[blockNumber]].staAdr);
    program_counter += (halfBytes==5)?4:3;

    if(halfBytes==3){
      int relAdr = operandAddress - program_counter;
      if(relAdr>=(-2048) && relAdr<=2047){
        object_code = intToHexString(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+2,2);
        object_code += '2';
        object_code += intToHexString(relAdr,halfBytes);
        return object_code;
      }

      if(!nobase){
        relAdr = operandAddress - base_register_value;
        if(relAdr>=0 && relAdr<=4095){
          object_code = intToHexString(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+2,2);
          object_code += '4';
          object_code += intToHexString(relAdr,halfBytes);
          return object_code;
        }
      }

      if(operandAddress<=4095){
        object_code = intToHexString(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+2,2);
        object_code += '0';
        object_code += intToHexString(operandAddress,halfBytes);

        /*add modifacation record here*/
        modifi_rec += "M^" + intToHexString(address+1+stringHexToInt(BLOCKS[BLocksNumToName[blockNumber]].staAdr),6) + '^';
        modifi_rec += (halfBytes==5)?"05":"03";
        modifi_rec += '\n';

        return object_code;
      }
    }
    else{//No base or pc based addressing in format 4
      object_code = intToHexString(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+2,2);
      object_code += '1';
      object_code += intToHexString(operandAddress,halfBytes);

      /*add modifacation record here*/
      modifi_rec += "M^" + intToHexString(address+1+stringHexToInt(BLOCKS[BLocksNumToName[blockNumber]].staAdr),6) + '^';
      modifi_rec += (halfBytes==5)?"05":"03";
      modifi_rec += '\n';

      return object_code;
    }

    writeData = "Line: "+to_string(lineNumber);
    writeData += "Can't fit into program counter based or base register based addressing.";
    writeToFile(errorFile,writeData);
    object_code = intToHexString(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+2,2);
    object_code += (halfBytes==5)?"100000":"0000";

    return object_code;
  }
  else if(getFlagFormat(operand)=='='){//Literals
    string tempOperand = operand.substr(1,operand.length()-1);

    if(tempOperand=="*"){
      tempOperand = "X'" + intToHexString(address,6) + "'";
      /*Add modification record for value created by operand */
      modifi_rec += "M^" + intToHexString(stringHexToInt(LITTAB[tempOperand].address)+stringHexToInt(BLOCKS[BLocksNumToName[LITTAB[tempOperand].blockNumber]].staAdr),6) + '^';
      modifi_rec += intToHexString(6,2);
      modifi_rec += '\n';
    }

    if(LITTAB[tempOperand].exists=='n'){
      writeData = "Line "+to_string(lineNumber)+" : Symbol doesn't exists. Found " + tempOperand;
      writeToFile(errorFile,writeData);

      object_code = intToHexString(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
      object_code += (halfBytes==5)?"000":"0";
      object_code += "000";
      return object_code;
    }

    int operandAddress = stringHexToInt(LITTAB[tempOperand].address) + stringHexToInt(BLOCKS[BLocksNumToName[LITTAB[tempOperand].blockNumber]].staAdr);
    program_counter = address + stringHexToInt(BLOCKS[BLocksNumToName[blockNumber]].staAdr);
    program_counter += (halfBytes==5)?4:3;

    if(halfBytes==3){
      int relAdr = operandAddress - program_counter;
      if(relAdr>=(-2048) && relAdr<=2047){
        object_code = intToHexString(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
        object_code += '2';
        object_code += intToHexString(relAdr,halfBytes);
        return object_code;
      }

      if(!nobase){
        relAdr = operandAddress - base_register_value;
        if(relAdr>=0 && relAdr<=4095){
          object_code = intToHexString(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
          object_code += '4';
          object_code += intToHexString(relAdr,halfBytes);
          return object_code;
        }
      }

      if(operandAddress<=4095){
        object_code = intToHexString(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
        object_code += '0';
        object_code += intToHexString(operandAddress,halfBytes);

        /*add modifacation record here*/
        modifi_rec += "M^" + intToHexString(address+1+stringHexToInt(BLOCKS[BLocksNumToName[blockNumber]].staAdr),6) + '^';
        modifi_rec += (halfBytes==5)?"05":"03";
        modifi_rec += '\n';

        return object_code;
      }
    }
    else{//No base or pc based addressing in format 4
      object_code = intToHexString(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
      object_code += '1';
      object_code += intToHexString(operandAddress,halfBytes);

      /*add modifacation record here*/
      modifi_rec += "M^" + intToHexString(address+1+stringHexToInt(BLOCKS[BLocksNumToName[blockNumber]].staAdr),6) + '^';
      modifi_rec += (halfBytes==5)?"05":"03";
      modifi_rec += '\n';

      return object_code;
    }

    writeData = "Line: "+to_string(lineNumber);
    writeData += "Can't fit into program counter based or base register based addressing.";
    writeToFile(errorFile,writeData);
    object_code = intToHexString(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
    object_code += (halfBytes==5)?"100":"0";
    object_code += "000";

    return object_code;
  }
  else{/*Handle direct addressing*/
    int xbpe=0;
    string tempOperand = operand;
    if(operand.substr(operand.length()-2,2)==",X"){
      tempOperand = operand.substr(0,operand.length()-2);
      xbpe = 8;
    }


    if(SYMTAB[tempOperand].exists=='n') {
      /*****/
      if(halfBytes==3) { 
      writeData = "Line "+to_string(lineNumber);
      writeData += " : Symbol doesn't exists. Found " + tempOperand;
      writeToFile(errorFile,writeData);
      object_code = intToHexString(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
      object_code += (halfBytes==5)?(intToHexString(xbpe+1,1)+"00"):intToHexString(xbpe,1);
      object_code += "000";
      return object_code;
    }
    }
else{
    int operandAddress = stringHexToInt(SYMTAB[tempOperand].address) + stringHexToInt(BLOCKS[BLocksNumToName[SYMTAB[tempOperand].blockNumber]].staAdr);
    program_counter = address + stringHexToInt(BLOCKS[BLocksNumToName[blockNumber]].staAdr);
    program_counter += (halfBytes==5)?4:3;

    if(halfBytes==3){
      int relAdr = operandAddress - program_counter;
      if(relAdr>=(-2048) && relAdr<=2047){
        object_code = intToHexString(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
        object_code += intToHexString(xbpe+2,1);
        object_code += intToHexString(relAdr,halfBytes);
        return object_code;
      }

      if(!nobase){
        relAdr = operandAddress - base_register_value;
        if(relAdr>=0 && relAdr<=4095){
          object_code = intToHexString(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
          object_code += intToHexString(xbpe+4,1);
          object_code += intToHexString(relAdr,halfBytes);
          return object_code;
        }
      }

      if(operandAddress<=4095){
        object_code = intToHexString(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
        object_code += intToHexString(xbpe,1);
        object_code += intToHexString(operandAddress,halfBytes);

        /*add modifacation record here*/
        modifi_rec += "M^" + intToHexString(address+1+stringHexToInt(BLOCKS[BLocksNumToName[blockNumber]].staAdr),6) + '^';
        modifi_rec += (halfBytes==5)?"05":"03";
        modifi_rec += '\n';

        return object_code;
      }
    }
    else{//No base or pc based addressing in format 4
      object_code = intToHexString(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
      object_code += intToHexString(xbpe+1,1);
      object_code += intToHexString(operandAddress,halfBytes);

      /*add modifacation record here*/
      modifi_rec += "M^" + intToHexString(address+1+stringHexToInt(BLOCKS[BLocksNumToName[blockNumber]].staAdr),6) + '^';
      modifi_rec += (halfBytes==5)?"05":"03";
      modifi_rec += '\n';

      return object_code;
    }

    writeData = "Line: "+to_string(lineNumber);
    writeData += "Can't fit into program counter based or base register based addressing.";
    writeToFile(errorFile,writeData);
    object_code = intToHexString(stringHexToInt(OPTAB[getRealOpcode(opcode)].opcode)+3,2);
    object_code += (halfBytes==5)?(intToHexString(xbpe+1,1)+"00"):intToHexString(xbpe,1);
    object_code += "000";

    return object_code;
  }}
  return "AB";
}

void writeTextRecord(bool lastRecord=false){
  if(lastRecord){
    if(currentRecord.length()>0){//Write last text record
      writeData = intToHexString(currentRecord.length()/2,2) + '^' + currentRecord;
      writeToFile(objectFile,writeData);
      currentRecord = "";
    }
    return;
  }
  if(objectCode != ""){
    if(currentRecord.length()==0){
      writeData = "T^" + intToHexString(address+stringHexToInt(BLOCKS[BLocksNumToName[blockNumber]].staAdr),6) + '^';
      writeToFile(objectFile,writeData,false);
    }
    //What is objectCode length > 60
    if((currentRecord + objectCode).length()>60){
      //Write current record
      writeData = intToHexString(currentRecord.length()/2,2) + '^' + currentRecord;
      writeToFile(objectFile,writeData);

      //Initialize new text currentRecord
      currentRecord = "";
      writeData = "T^" + intToHexString(address+stringHexToInt(BLOCKS[BLocksNumToName[blockNumber]].staAdr),6) + '^';
      writeToFile(objectFile,writeData,false);
    }

    currentRecord += objectCode;
  }
  else{
    /*Assembler directive which doesn't result in address genrenation*/
    if(opcode=="START"||opcode=="END"||opcode=="BASE"||opcode=="NOBASE"||opcode=="LTORG"||opcode=="ORG"||opcode=="EQU"){
      /*DO nothing*/
    }
    else{
      //Write current record if exists
      if(currentRecord.length()>0){
        writeData = intToHexString(currentRecord.length()/2,2) + '^' + currentRecord;
        writeToFile(objectFile,writeData);
      }
      currentRecord = "";
    }
  }
}

void writeEndRecord(bool write=true){
  if(write){
    if(endRecord.length()>0){
      writeToFile(objectFile,endRecord);
     
    }
    else{
      writeEndRecord(false);
    }
  }
  if((operand==""||operand==" ")&&currentSectName=="DEFAULT"){//If no operand of END
    endRecord = "E^" + intToHexString(startAddress,6);
  }else if(currentSectName!="DEFAULT"){
    endRecord = "E";
  }
  else{//Make operand on end firstExecutableAddress
    int firstExecutableAddress;
   
      firstExecutableAddress = stringHexToInt(SYMTAB[firstExecutable_Sec].address);
    
    endRecord = "E^" + intToHexString(firstExecutableAddress,6)+"\n";
  }
  
}

void pass2(){
  string tempBuffer;
  intermediateFile.open("intermediate_"+fileName);//begin
  if(!intermediateFile){
    cout << "Unable to open file: intermediate_"<<fileName<<endl;
    exit(1);
  }
  getline(intermediateFile, tempBuffer); // Discard heading line

  objectFile.open("object_"+fileName);
  if(!objectFile){
    cout<<"Unable to open file: object_"<<fileName<<endl;
    exit(1);
  }

  ListingFile.open("listing_"+fileName);
  if(!ListingFile){
    cout<<"Unable to open file: listing_"<<fileName<<endl;
    exit(1);
  }
  writeToFile(ListingFile,"Line\tAddress\tLabel\tOPCODE\tOPERAND\tObjectCode\tComment");

  errorFile.open("error_"+fileName,fstream::app);
  if(!errorFile){
    cout<<"Unable to open file: error_"<<fileName<<endl;
    exit(1);
  }
  writeToFile(errorFile,"\n\n************PASS2************");
   /*Intialize global variables*/
  objectCode = "";
  currentTextRecordLength=0;
  currentRecord = "";
  modifi_rec = "";
  blockNumber = 0;
  nobase = true;

  readIntermediateFile(intermediateFile,isComment,lineNumber,address,blockNumber,label,opcode,operand,comment);
  while(isComment){//Handle with previous comments
    writeData = to_string(lineNumber) + "\t" + comment;
    writeToFile(ListingFile,writeData);
    readIntermediateFile(intermediateFile,isComment,lineNumber,address,blockNumber,label,opcode,operand,comment);
  }

  if(opcode=="START"){
    startAddress = address;
    writeData = to_string(lineNumber) + "\t" + intToHexString(address) + "\t" + to_string(blockNumber) + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + objectCode +"\t" + comment;
    writeToFile(ListingFile,writeData);
  }
  else{
    label = "";
    startAddress = 0;
    address = 0;
  }
    program_section_length = program_length ;

  writeData = "H^"+expandString(label,6,' ',true)+'^'+intToHexString(address,6)+'^'+intToHexString(program_section_length,6);
  writeToFile(objectFile,writeData);
 
  readIntermediateFile(intermediateFile,isComment,lineNumber,address,blockNumber,label,opcode,operand,comment);
  currentTextRecordLength  = 0;
  while(opcode!="END"){
      while(opcode!="END"){
    if(!isComment){
      if(OPTAB[getRealOpcode(opcode)].exists=='y'){
        if(OPTAB[getRealOpcode(opcode)].format==1){
          objectCode = OPTAB[getRealOpcode(opcode)].opcode;
        }
        else if(OPTAB[getRealOpcode(opcode)].format==2){
          operand1 = operand.substr(0,operand.find(','));
          operand2 = operand.substr(operand.find(',')+1,operand.length()-operand.find(',') -1 );

          if(operand2==operand){//If not two operand i.e. a
            if(getRealOpcode(opcode)=="SVC"){
              objectCode = OPTAB[getRealOpcode(opcode)].opcode + intToHexString(stoi(operand1),1) + '0';
            }
            else if(REGTAB[operand1].exists=='y'){
              objectCode = OPTAB[getRealOpcode(opcode)].opcode + REGTAB[operand1].num + '0';
            }
            else{
              objectCode = getRealOpcode(opcode) + '0' + '0';
              writeData = "Line: "+to_string(lineNumber)+" Invalid Register name";
              writeToFile(errorFile,writeData);
            }
          }
          else{//Two operands i.e. a,b
            if(REGTAB[operand1].exists=='n'){
              objectCode = OPTAB[getRealOpcode(opcode)].opcode + "00";
              writeData = "Line: "+to_string(lineNumber)+" Invalid Register name";
              writeToFile(errorFile,writeData);
            }
            else if(getRealOpcode(opcode)=="SHIFTR" || getRealOpcode(opcode)=="SHIFTL"){
              objectCode = OPTAB[getRealOpcode(opcode)].opcode + REGTAB[operand1].num + intToHexString(stoi(operand2),1);
            }
            else if(REGTAB[operand2].exists=='n'){
              objectCode = OPTAB[getRealOpcode(opcode)].opcode + "00";
              writeData = "Line: "+to_string(lineNumber)+" Invalid Register name";
              writeToFile(errorFile,writeData);
            }
            else{
              objectCode = OPTAB[getRealOpcode(opcode)].opcode + REGTAB[operand1].num + REGTAB[operand2].num;
            }
          }
        }
        else if(OPTAB[getRealOpcode(opcode)].format==3){
          if(getRealOpcode(opcode)=="RSUB"){
            objectCode = OPTAB[getRealOpcode(opcode)].opcode;
            objectCode += (getFlagFormat(opcode)=='+')?"000000":"0000";
          }
          else{
            objectCode = createObjectCodeFormat34();
          }
        }
      }//If opcode in optab
      else if(opcode=="BYTE"){
        if(operand[0]=='X'){
          objectCode = operand.substr(2,operand.length()-3);
        }
        else if(operand[0]=='C'){
          objectCode = stringToHexString(operand.substr(2,operand.length()-3));
        }
      }
      else if(label=="*"){
        if(opcode[1]=='C'){
          objectCode = stringToHexString(opcode.substr(3,opcode.length()-4));
        }
        else if(opcode[1]=='X'){
          objectCode = opcode.substr(3,opcode.length()-4);
        }
      }
      else if(opcode=="WORD"){
        objectCode = intToHexString(stoi(operand),6);
      }
      else if(opcode=="BASE"){
        if(SYMTAB[operand].exists=='y'){
          base_register_value = stringHexToInt(SYMTAB[operand].address) + stringHexToInt(BLOCKS[BLocksNumToName[SYMTAB[operand].blockNumber]].staAdr);
          nobase = false;
        }
        else{
          writeData = "Line "+to_string(lineNumber)+" : Symbol doesn't exists. Found " + operand;
          writeToFile(errorFile,writeData);
        }
        objectCode = "";
      }
      else if(opcode=="NOBASE"){
        if(nobase){//check if assembler was using base addressing
          writeData = "Line "+to_string(lineNumber)+": Assembler wasn't using base addressing";
          writeToFile(errorFile,writeData);
        }
        else{
          nobase = true;
        }
        objectCode = "";
      }
      else{
        objectCode = "";
      }
      //Write to text record if any generated
      writeTextRecord();

      if(blockNumber==-1 && address!=-1){
        writeData = to_string(lineNumber) + "\t" + intToHexString(address) + "\t" + " " + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + objectCode +"\t" + comment;
      }
      else if(address==-1){
        writeData = to_string(lineNumber) + "\t" + " " + "\t" + " " + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + objectCode +"\t" + comment;
      } 
     
      else{ writeData = to_string(lineNumber) + "\t" + intToHexString(address) + "\t" + to_string(blockNumber) + "\t" + label + "\t" + opcode + "\t" + operand + "\t" + objectCode +"\t" + comment;
      }
    }//if not comment
    else{
      writeData = to_string(lineNumber) + "\t" + comment;
    }
    writeToFile(ListingFile,writeData);//Write listing line
    readIntermediateFile(intermediateFile,isComment,lineNumber,address,blockNumber,label,opcode,operand,comment);//Read next line
    objectCode = "";
  }//while opcode not end
  writeTextRecord();

  //Save end record
  writeEndRecord(false);
  }
if(opcode!="CSECT"){
  while(readIntermediateFile(intermediateFile,isComment,lineNumber,address,blockNumber,label,opcode,operand,comment)){
    if(label=="*"){
      if(opcode[1]=='C'){
        objectCode = stringToHexString(opcode.substr(3,opcode.length()-4));
      }
      else if(opcode[1]=='X'){
        objectCode = opcode.substr(3,opcode.length()-4);
      }
      writeTextRecord();
    }
    writeData = to_string(lineNumber) + "\t" + intToHexString(address) + "\t" + to_string(blockNumber) + label + "\t" + opcode + "\t" + operand + "\t" + objectCode +"\t" + comment;
    writeToFile(ListingFile,writeData);
  }
}   
  
writeTextRecord(true);
if(!isComment){
  
  writeToFile(objectFile,modifi_rec,false);//Write modification record
  writeEndRecord(true);//Write end record
  currentSectName=label;
  modifi_rec="";
}
  readIntermediateFile(intermediateFile,isComment,lineNumber,address,blockNumber,label,opcode,operand,comment);//Read next line
    objectCode = "";

}



int main(){
  cout<<"****Input file and executable(assembler.out) should be in same folder****"<<endl<<endl;
  cout<<"Enter name of input file:";
  cin>>fileName;

  cout<<"\nLoading OPTAB"<<endl;
  loadTables();

  cout<<"\nPerforming PASS1"<<endl;
  cout<<"Writing intermediate file to 'intermediate_"<<fileName<<"'"<<endl;
  cout<<"Writing error file to 'error_"<<fileName<<"'"<<endl;
  pass1();

cout<<"Writing SYMBOL TABLE"<<endl;
  printtab.open("tables_"+fileName) ;
  writeToFile(printtab,"**********************************SYMBOL TABLE*****************************\n") ;
    for (auto const& it: SYMTAB) { 
        writestring+=it.first+":-\t"+ "name:"+it.second.name+"\t|"+ "address:"+it.second.address+"\t|"+ "relative:"+intToHexString(it.second.relative)+" \n" ;
    } 
    writeToFile(printtab,writestring) ;

writestring="" ;
    cout<<"Writing LITERAL TABLE"<<endl;
  
  writeToFile(printtab,"**********************************LITERAL TABLE*****************************\n") ;
    for (auto const& it: LITTAB) { 
        writestring+=it.first+":-\t"+ "value:"+it.second.value+"\t|"+ "address:"+it.second.address+" \n" ;
    } 
    writeToFile(printtab,writestring) ;

  cout<<"\nPerforming PASS2"<<endl;
  cout<<"Writing object file to 'object_"<<fileName<<"'"<<endl;
  cout<<"Writing listing file to 'listing_"<<fileName<<"'"<<endl;
  pass2();

}
