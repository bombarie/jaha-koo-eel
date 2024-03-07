void updateSerialIO();
void checkIncomingSerial();

uint8_t channelToPrint = 255;
void updateSerialIO()
{
    checkIncomingSerial();

    EVERY_N_MILLIS(100)
    {
        if (channelToPrint == 0) // only nood1a
        {
            Serial.print("n00d1a: ");
            Serial.print(n00d1a);
            Serial.print("\t in binary: ");
            Serial.println(n00d1a, BIN);
        }
        if (channelToPrint == 1) // only nood1b
        {
            Serial.print("n00d1b: ");
            Serial.print(n00d1b);
            Serial.print("\t in binary: ");
            Serial.println(n00d1b, BIN);
        }
        if (channelToPrint == 2) // only nood2a
        {
            if (throttle & (0x1 << 7)) // 128
            {
                Serial.print("TX_THROTTLE: ");
                Serial.print(data.ch[TX_THROTTLE]);
                Serial.print("\t throttle: ");
                Serial.print(throttle);
                Serial.print("\t (in binary: ");
                Serial.print(throttle, BIN);
            }
            Serial.print("\t n00d2a: ");
            Serial.print(n00d2a);
            Serial.print("\t in binary: ");
            Serial.println(n00d2a, BIN);
        }
        if (channelToPrint == 3) // only nood2b
        {
            if (throttle & (0x1 << 6)) // 64
            {
                Serial.print("TX_THROTTLE: ");
                Serial.print(data.ch[TX_THROTTLE]);
                Serial.print("\t throttle: ");
                Serial.print(throttle);
                Serial.print("\t (in binary: ");
                Serial.print(throttle, BIN);
            }
            Serial.print("n00d2b: ");
            Serial.print(n00d2b);
            Serial.print("\t in binary: ");
            Serial.println(n00d2b, BIN);
        }
        if (channelToPrint == 100) // everything
        {
            Serial.print("TX_THROTTLE: ");
            Serial.print(data.ch[TX_THROTTLE]);
            Serial.print("\t throttle: ");
            Serial.print(throttle);
            Serial.print("\t (in binary: ");
            Serial.println(throttle, BIN);
            Serial.print("n00d1a: ");
            Serial.print(n00d1a);
            Serial.print("\t n00d1b: ");
            Serial.print(n00d1b);
            Serial.print("\t n00d2a: ");
            Serial.print(n00d2a);
            Serial.print("\t n00d2b: ");
            Serial.println(n00d2b);
            Serial.println("");
        }
    }
}

void checkIncomingSerial()
{
    if (Serial.available() > 0)
    {
        char inChar = Serial.read();
        switch (inChar)
        {
        case '1':
            channelToPrint = 0;
            break;
        case '2':
            channelToPrint = 1;
            break;
        case '3':
            channelToPrint = 2;
            break;
        case '4':
            channelToPrint = 3;
            break;
        case 'a':
            channelToPrint = 100;
            break;
        case 'x':
            channelToPrint = 255;
            break;
        }

        while (Serial.available() > 0)
        {
            Serial.read();
        }
    }
}
