template<class T>
void dump(T out, bool newLine = true){
    if (isDebug){
        if (newLine){
            Serial.println(out);
        }
        else{
            Serial.print(out);
        }
    }
}