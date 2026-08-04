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
            Serial.print("nood1a: ");
            Serial.print(noodVals[0]);
            Serial.print("\t in binary: ");
            Serial.print(noodVals[0], BIN);
            Serial.print("\t throttle in binary: ");
            Serial.println(throttle, BIN);
        }
        if (channelToPrint == 1) // only nood1b
        {
            Serial.print("nood1b: ");
            Serial.print(noodVals[1]);
            Serial.print("\t in binary: ");
            Serial.print(noodVals[1], BIN);
            Serial.print("\t throttle in binary: ");
            Serial.println(throttle, BIN);
        }
        if (channelToPrint == 2) // only nood2a
        {
            Serial.print("\t nood2a: ");
            Serial.print(noodVals[2]);
            Serial.print("\t in binary: ");
            Serial.print(nood2a, BIN);
            Serial.print("\t throttle in binary: ");
            Serial.println(throttle, BIN);
        }
        if (channelToPrint == 3) // only nood2b
        {
            Serial.print("nood2b: ");
            Serial.print(noodVals[3]);
            Serial.print("\t in binary: ");
            Serial.print(noodVals[3], BIN);
            Serial.print("\t throttle in binary: ");
            Serial.println(throttle, BIN);
        }
        if (channelToPrint == 90) // just throttle binary and selectedChannel
        {
            // Serial.print("Throttle (bin): ");
            // Serial.print(throttle, BIN);
            // Serial.println("\t currSelectedChannel: " + String(currSelectedChannel) + "\t channelChangeCounter: " + String(channelChangeCounter) + String(")"));
            Serial.print(throttle, BIN);
            Serial.println("\t" + String(currSelectedChannel) + "\t" + String(channelChangeCounter));
        }
    }

    EVERY_N_MILLIS(25)
    {
        if (channelToPrint == 100) // everything
        {
            Serial.print("TX_THROTTLE: ");
            Serial.print(data.ch[TX_THROTTLE]);
            Serial.print("\t throttle: ");
            Serial.print(throttle);
            Serial.print("\t");
            Serial.println(throttle, BIN);
            Serial.print("throttleSmoothed: ");
            Serial.print(throttleSmoothed);
            Serial.print("\t");
            Serial.println(throttleSmoothed, BIN);
            Serial.print("nood1a: ");
            Serial.print(noodVals[0]);
            Serial.print("\t nood1b: ");
            Serial.print(noodVals[1]);
            Serial.print("\t nood2a: ");
            Serial.print(noodVals[2]);
            Serial.print("\t nood2b: ");
            Serial.println(noodVals[3]);
            Serial.print("nood1a_smooth: ");
            Serial.print(noodAvgVals[0]);
            Serial.print("\t nood1b_smooth: ");
            Serial.print(noodAvgVals[1]);
            Serial.print("\t nood2a_smooth: ");
            Serial.print(noodAvgVals[2]);
            Serial.print("\t nood2b_smooth: ");
            Serial.println(noodAvgVals[3]);
            Serial.println("currSelectedChannel: " + String(currSelectedChannel) + "\t channelChangeCounter: " + String(channelChangeCounter));
            Serial.println("");
        }
    }

    EVERY_N_MILLIS(4)
    {
        if (channelToPrint == 101) // everything (TelePlot)
        {
           
            Serial.print(">thr:");
            Serial.println(throttle);
            Serial.print(">thrSmth:");
            Serial.println(throttleSmoothed);

            Serial.print(">nood1a:");
            Serial.println(noodVals[0]);
            Serial.print(">nood1b:");
            Serial.println(noodVals[1]);
            Serial.print(">nood2a:");
            Serial.println(noodVals[2]);
            Serial.print(">nood2b:");
            Serial.println(noodVals[3]);

            Serial.print(">nood1a_smth:");
            Serial.println(noodAvgVals[0]);
            Serial.print(">nood1b_smth:");
            Serial.println(noodAvgVals[1]);
            Serial.print(">nood2a_smth:");
            Serial.println(noodAvgVals[2]);
            Serial.print(">nood2b_smth:");
            Serial.println(noodAvgVals[3]);

            Serial.print(">calibLowerBound:");
            Serial.println(calibLowerBound);
            Serial.print(">calibUpperBound:");
            Serial.println(calibUpperBound);
            Serial.print(">calibThreshold:");
            Serial.println(calibThreshold);
            Serial.print(">calibratedThrottle:");
            Serial.println(calibratedThrottle);
            Serial.print(">calibDetectedChannel:");
            Serial.println(calibDetectedChannel);
            Serial.print(">calibDetectedChannelSmooth:");
            Serial.println(calibDetectedChannelSmooth);

            Serial.print(">calibState:");
            Serial.println(calibState);

            Serial.print(">currSelectedCh:");
            Serial.println(currSelectedChannel);
            Serial.print(">currDetectedCh:");
            Serial.println(currDetectedChannelSmooth);
            Serial.print(">currDetectedChFst:");
            Serial.println(currDetectedChannel);
            Serial.print(">channelChangeCtr:");
            Serial.println(channelChangeCounter);
        }
    }
}

void checkIncomingSerial()
{
    if (Serial.available() > 0)
    {
        printSbusData = false;

        char inChar = Serial.read();
        switch (inChar)
        {
        case '1': // nood1a
            Serial.println("serial printing only nood1a");
            channelToPrint = 0;
            break;
        case '2': // nood1b
            Serial.println("serial printing only nood1b");
            channelToPrint = 1;
            break;
        case '3': // nood2a
            Serial.println("serial printing only nood2a");
            channelToPrint = 2;
            break;
        case '4': // nood2b
            Serial.println("serial printing only nood2b");
            channelToPrint = 3;
            break;
        case 't': // just throttle and selectedChannel
            Serial.println("serial printing just throttle and selectedChannel");
            Serial.print("thr: ");
            Serial.print("\t chan: ");
            Serial.println("\t next chan counter:");
            channelToPrint = 90;
            break;

        case 'l': // DEBUG
            bListenForIncomingCalibration = !bListenForIncomingCalibration;
            Serial.println("set bListenForIncomingCalibration to " + String(bListenForIncomingCalibration));
            break;

            case 'r': // DEBUG - reset currSelectedChannel, currDetectedChannel, prevDetectedChannel and nextChannelIndex
            currSelectedChannel = 0;
            currDetectedChannelSmooth = 0;
            prevDetectedChannel = 0;
            nextChannelIndex = 1;
            break;
        case '[': // shift channelChangeThreshold down
            if (channelChangeThreshold > 0)
            {
                channelChangeThreshold--;
            }
            Serial.println("Set channelChangeThreshold to " + String(channelChangeThreshold));
            break;
        case ']': // shift channelChangeThreshold up
            channelChangeThreshold++;
            Serial.println("Set channelChangeThreshold to " + String(channelChangeThreshold));
            break;
        case '<': // shift throttleSmoothFactor down
            if (throttleSmoothFactor > 0.0)
            {
                throttleSmoothFactor -= .02;
            }
            Serial.println("Set throttleSmoothFactor to " + String(throttleSmoothFactor));
            break;
        case '>': // shift throttleSmoothFactor up
            if (throttleSmoothFactor < 1.0)
            {
                throttleSmoothFactor += .02;
            }
            Serial.println("Set throttleSmoothFactor to " + String(throttleSmoothFactor));
            break;

        case '7':
            Serial.println("change elrs parse method to doDirectChange()");
            ELRSParseMethod = 0;
            break;
        case '8':
            Serial.println("change elrs parse method to doThresholdChange()");
            ELRSParseMethod = 1;
            break;
        case '9':
            Serial.println("change elrs parse method to doNextItemChange()");
            ELRSParseMethod = 2;
            break;
        case '0':
            Serial.println("change elrs parse method to doThresholdChangeCalibrated()");
            ELRSParseMethod = 3;
            break;

        case 'a': // all noods
            Serial.println("serial printing all noods");
            channelToPrint = 100;
            break;
        case 'A': // all noods (using TelePlot)
            Serial.println("serial printing all noods - targetting TelePlot");
            channelToPrint = 101;

            Serial.println("throttleSmoothFactor:" + String(throttleSmoothFactor));
            Serial.println("channelChangeThreshold:" + String(channelChangeThreshold));
            break;

            case 's': // sbus
            Serial.println("printing incoming sbus data");
            printSbusData = true;
            channelToPrint = 255;
            break;
        case 'x': // nothing
            Serial.println("serial printing no noods (and no sbus data)");
            channelToPrint = 255;
            break;
        }

        while (Serial.available() > 0)
        {
            Serial.read();
        }
    }
}
