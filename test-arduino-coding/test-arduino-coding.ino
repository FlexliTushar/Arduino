// #include <StandardCplusplus.h>
#include <string>
#include<vector>
using namespace std;

string message = "Hello Arduino";
vector<string> vec(4, message);

void setup() 
{
  Serial.begin(9600);

  Serial.println(message.c_str());
  Serial.print("Length: ");
  Serial.println(message.length());  

  string subString = message.substr(6,5);
  Serial.println(subString.c_str());
}

void loop() {}