/******************************************************************************
MIT License

Copyright (c) 2019 Cyrille Gaillardet

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
******************************************************************************/

#include <ir_Daikin.h>
#include <ArduinoJson.h>

#include <PubSubClient.h>
#include <ESP8266WebServer.h>

#include "aircon.h"

#include "settings.h"
#include <LogManagement.h>


DaikinAirConditioning::DaikinAirConditioning(int pin)
{
  _pDaikinAircon = new IRDaikinESP(pin);
  _temperature = DEFAULT_TEMP;
  _fan = DEFAULT_FAN_SPEED;
  _power = DEFAULT_POWER;
  _powerful = DEFAULT_POWERFUL;
  _quiet = DEFAULT_QUIET;
  _swingVertical = DEFAULT_SWING_V ;
  _programMode = DEFAULT_MODE;

  _FanSpeedStates[DAIKIN_FAN_AUTO]= "Auto";
  _FanSpeedStates[1]= "Slowest";
  _FanSpeedStates[2]= "Slow";
  _FanSpeedStates[3]= "Medium";
  _FanSpeedStates[4]= "Fast";
  _FanSpeedStates[5]= "Fastest";

  _ProgramStates[DAIKIN_COOL] = "Cool";
  _ProgramStates[DAIKIN_HEAT] = "Heat";
  _ProgramStates[DAIKIN_FAN] = "Fan";
  _ProgramStates[DAIKIN_AUTO] = "Auto";
  _ProgramStates[DAIKIN_DRY] = "Dry";
      
  _OnOffStates[0] = "Off";
  _OnOffStates[1] = "On";

  _actuatorType = ACTUATOR_IR;
  _actuatorName ="Daikin IR command";

    _stateTopic="STATE/PIN";

    char tmpbuf[10];
    sprintf(tmpbuf,"%d",pin);
    _stateTopic.append(tmpbuf);
    _stateTopic.append("_AIRCON");
    _setTopic="SET/PIN";
    _setTopic.append(tmpbuf);
    _setTopic.append("_AIRCON");
  
}

DaikinAirConditioning::~DaikinAirConditioning(void)
{
  delete _pDaikinAircon;
}


int DaikinAirConditioning::GetStateFromString(std::map<int,std::string> &state, std::string &str)
{
  std::map<int,std::string>::iterator it; 
  for(it=state.begin(); it!=state.end();it++)
  {
    if (it->second == str)
    {
      return it->first;
    }
  }
  /* this should not happen */
  return SELECTION_ERROR;
}


std::string * DaikinAirConditioning::GenerateSelection(std::map<int,std::string> &state, char *settingName, int selected)
{
  std::string *pRet= new std::string("<TD>");
  pRet->append(settingName);
  char localConvertBuffer[10]= {0};
  
  pRet->append(":</TD><TD><select name=\"");
  pRet->append(settingName);
  pRet->append( "\">");

  std::map<int,std::string>::iterator it;

  for (it=state.begin();it!=state.end();it++)
  {
    pRet->append("<option ");
      if (it->first == selected)
      {
         pRet->append("selected ");
      }
      pRet->append("value = ");
      itoa (it->first,localConvertBuffer,10);
      pRet->append(localConvertBuffer);
      pRet->append(" > ");
      pRet->append(it->second);
      pRet->append("</option>");
  }
  pRet->append("</select></TD>");

  return pRet;
}

std::string* DaikinAirConditioning::GenerateFanSelection(void)
{
  return GenerateSelection( _FanSpeedStates, "Fan speed",_fan);
}

std::string* DaikinAirConditioning::GenerateProgramSelection(void)
{
  return GenerateSelection(_ProgramStates, "Mode",_programMode);
}

std::string* DaikinAirConditioning::GeneratePowerSelection(void)
{
  return GenerateSelection(_OnOffStates, "Power",_power);
}

std::string* DaikinAirConditioning::GeneratePowerfulSelection(void)
{
  return GenerateSelection(_OnOffStates, "Powerful",_powerful);
}

std::string* DaikinAirConditioning::GenerateSwingSelection(void)
{
  return GenerateSelection(_OnOffStates, "Vertical swing",_swingVertical);
}

std::string* DaikinAirConditioning::GenerateQuietSelection(void)
{
  return GenerateSelection(_OnOffStates, "Quiet",_quiet);
}

std::string* DaikinAirConditioning::GenerateTemperatureSelection(void)
{
  std::string *pRet= new std::string("<TD>Temperature:</TD><TD><select name=\"Temperature\">");
  char localConvertBuffer[10]= {0};
  
  for (int i = DAIKIN_MIN_TEMP; i <= DAIKIN_MAX_TEMP; i++)
  {
    pRet->append("<option ");
    if (i == _temperature)
    {
       pRet->append("selected ");
    }

    pRet->append("value = ");
    itoa (i,localConvertBuffer,10);
    pRet->append(localConvertBuffer);
    pRet->append(">");
    pRet->append(localConvertBuffer);
       
    pRet->append(" </option>");
  }
  pRet->append("</select></TD>");

  return pRet;
}


void DaikinAirConditioning::SendAirConditioningCommand(void)
{
  if (_pDaikinAircon != NULL)
  {
    _pDaikinAircon->setTemp(this->_temperature);
    _pDaikinAircon->setFan(this->_fan);
    _pDaikinAircon->setPower(this->_power);
    _pDaikinAircon->setPowerful(this->_powerful);
    _pDaikinAircon->setQuiet(this->_quiet);
    _pDaikinAircon->setSwingVertical(this->_swingVertical);
    _pDaikinAircon->setMode(this->_programMode);   
    _pDaikinAircon->send();
    LOG_LN("SendAirConditioningCommand successful")
  } else  {

    LOG_LN("SendAirConditioningCommand: _pDaikinAircon NULL")
  }
}

void DaikinAirConditioning::PublishMqttState(PubSubClient &mqttClient)
{
  const int JSON_BUFFER_SIZE = JSON_OBJECT_SIZE(10);
  
  StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;

  JsonObject& airconRoot = jsonBuffer.createObject();

  airconRoot["Aircon"]["temperature"] = _temperature;
  airconRoot["Aircon"]["fan"] = _fan;
  airconRoot["Aircon"]["power"] = _power;
  airconRoot["Aircon"]["powerful"] = _powerful;
  airconRoot["Aircon"]["quiet"] = _quiet;
  airconRoot["Aircon"]["swingVertical"] = _swingVertical;
  airconRoot["Aircon"]["programMode"] = _programMode;
  
  char buffer[airconRoot.measureLength() + 1];
  airconRoot.printTo(buffer, sizeof(buffer));

  mqttClient.publish(_stateTopic.c_str(), buffer, true);
  
  jsonBuffer.clear();
}

bool DaikinAirConditioning::ProcessMqttCommand(char *message)
{
  const int JSON_BUFFER_SIZE = JSON_OBJECT_SIZE(10);

  StaticJsonBuffer<JSON_BUFFER_SIZE> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(message);

  if (!root.success())
  {
    LOG_LN("parseObject() failed")
    return false;
  }
  
  const char* res = NULL;
  unsigned long int value = atoi(res);

  if ((res = root["temperature"]) && (value>= DAIKIN_MIN_TEMP) && (value<= DAIKIN_MAX_TEMP))
    { _temperature = value;}
  if ((res = root["fan"]) && (_FanSpeedStates[value] != ""))
    { _fan = value;}
  if ((res = root["power"] ) && (_OnOffStates[value] != ""))
    { _power = value;}
  if ((res = root["powerful"]) && (_OnOffStates[value] != ""))
    { _powerful = value;}
  if ((res = root["quiet"]) && (_OnOffStates[value] != ""))
    { _quiet = value;}
  if ((res = root["swingVertical"]) && (_OnOffStates[value] != ""))
    { _swingVertical = value;}
  if ((res = root["programMode"]) && (_ProgramStates[value]!=""))
    _programMode = value;
  
  return true;
}

void DaikinAirConditioning::ProcessWebCommand(ESP8266WebServer *pWebServer)
{
  String argName;
  unsigned long webValue;
  LOG_LN("WebCommand")

  for (uint8_t i = 0; i < pWebServer->args(); i++)
  {
    argName = pWebServer->argName(i);
    webValue = atoi(pWebServer->arg(i).c_str());
    if ((argName == "Temperature") && (webValue>= DAIKIN_MIN_TEMP) && (webValue<= DAIKIN_MAX_TEMP))
      { LOG_LN("updated temperature command") _temperature = webValue;}
    else if ((argName == "Fan") && (_FanSpeedStates[webValue] != ""))
      { LOG_LN("updated fan command") _fan = webValue;}
    else if ((argName == "Power") && (_OnOffStates[webValue] != ""))
      { LOG_LN("updated power command") _power = webValue;}
    else if ((argName == "Powerful") && (_OnOffStates[webValue] != ""))
      { LOG_LN("updated powerful command") _powerful = webValue;}
    else if ((argName == "Quiet") && (_OnOffStates[webValue] != ""))
      { LOG_LN("updated _quiet command") _quiet = webValue;}
    else if ((argName == "Swing") && (_OnOffStates[webValue] != ""))
      { LOG_LN("updated swing command") _swingVertical = webValue;}
    else if ((argName == "Mode") && (_ProgramStates[webValue]!=""))
      {LOG_LN("updated program mode command") _programMode = webValue;}

  }
  SendAirConditioningCommand();
}

std::string *DaikinAirConditioning::GenerateWebData(void)
{
  std::string *pWebData = new std::string("<form method=\"POST\" action=\"command\"><TABLE><TR><TD>");

  /* add name */
  pWebData->append(_actuatorName);
  pWebData->append("<BR><BR> Mqtt state ");
  pWebData->append(_stateTopic);
  pWebData->append("<BR> Mqtt set ");
  pWebData->append(_setTopic);
  pWebData->append("<BR><BR></TR><TR>");
  std::string *pSelection = GeneratePowerSelection();
  pWebData->append(*pSelection);
  delete pSelection;
  pWebData->append("</TR><TR>");
  pSelection = GenerateTemperatureSelection();
  pWebData->append(*pSelection);
  delete pSelection;
  pWebData->append("</TR><TR>");
  pSelection = GenerateProgramSelection();
  pWebData->append(*pSelection);
  delete pSelection;
  pWebData->append("</TR><TR>");
  pSelection = GenerateFanSelection();
  pWebData->append(*pSelection);
  delete pSelection;
  pWebData->append("</TR><TR>");
  pSelection = GeneratePowerfulSelection();
  pWebData->append(*pSelection);
  delete pSelection;
  pWebData->append("</TR><TR>");
  pSelection = GenerateQuietSelection();
  pWebData->append(*pSelection);
  delete pSelection;
  pWebData->append("</TR><TR>");
  pSelection = GenerateSwingSelection();
  pWebData->append(*pSelection);
  delete pSelection;
  pWebData->append("</TR></TABLE><TR><BR><input type=\"submit\"></form></TD></TR></TABLE>");
  return pWebData;
}
  
