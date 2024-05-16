import cv2
import json
import time
import numpy as np



# parses the data and assigns it to a variable called jess_dict

def parseJson(file= 'video_states.json'):
    jsonFile = open(file)
    jsonFile = json.loads(jsonFile.read())
    return jsonFile

def UpdateValueByName(jsonData, _clipName, key, value):
    for reg in jsonData:
        if(reg['name'] == _clipName):
            reg[key] = value

def compare_frames(video_capture, frameN1, frameN2):
    # Convert frames to grayscale
    video_capture.set(cv2.CAP_PROP_POS_FRAMES, frameN1)
    ret2, frame1 = video_capture.read()
    video_capture.set(cv2.CAP_PROP_POS_FRAMES, frameN2)
    ret1, frame2 = video_capture.read()
    if(ret1 and ret2):
        gray_frame1 = cv2.cvtColor(frame1, cv2.COLOR_BGR2GRAY)
        gray_frame2 = cv2.cvtColor(frame2, cv2.COLOR_BGR2GRAY)
        diff = cv2.absdiff(gray_frame1, gray_frame2)
        diff = cv2.sumElems(diff)[0]
        red = cv2.cvtColor(gray_frame1, cv2.COLOR_GRAY2BGR)
        blue = red = cv2.cvtColor(gray_frame1, cv2.COLOR_GRAY2RGB)
        overlapImage = np.zeros((gray_frame1.shape[0], gray_frame1.shape[1], 3), dtype=np.uint8)
        overlapImage[:, :, 2] = gray_frame1 #red
        overlapImage[:, :, 0] = gray_frame2 #blue
    
    return overlapImage, diff


def main():
    font = cv2.FONT_HERSHEY_SIMPLEX 

    smData = parseJson()
    video_capture = cv2.VideoCapture('Video.mp4')
    if(video_capture.isOpened() & (smData != None)):

        possiblePositions = set()
        for segment in smData:
            possiblePositions.add(segment['targetPosition'])
        
        framesToCheck = []
        for position in possiblePositions:
            for segCompare in smData:
                if(segCompare['position'] == position):
                    for segCompareTo in smData:

                        if((segCompareTo['position'] == position) & (segCompare['isEarlyExit'] == False)):
                            frameComp1 = (segCompare,'startTime')
                            frameComp2 = (segCompareTo,'startTime')
                            # description = "Comparing: (" + segCompare['name'] + " startTime | " + segCompareTo['name'] + " startTime)"
                            framesToCheck.append((frameComp1, frameComp2))
                        
                        if(segCompareTo['targetPosition'] == position):
                            frameComp1 = (segCompare,'startTime')
                            frameComp2 = (segCompareTo,'endTime')
                            # description = "Comparing: (" + segCompare['name'] + " startTime | " + segCompareTo['name'] + " endTime)"
                            framesToCheck.append((frameComp1, frameComp2))
                        
                if(segCompare['isEarlyExit'] == True):

                    frameComp1 = (segCompare , 'startTime')
                    frameComp2 = (segCompare, 'transitionFromParent') 
                    # description = "Comparing: (" + segCompare['name'] + " startTime | " + segCompare['name'] + " transitionTime)"
                    framesToCheck.append((frameComp1, frameComp2))

                        

        index = 0
        frameComp1Offset = 0
        frameComp2Offset = 0
        isEditing = 0
        while True:
        # for frames in framesToCheck:
            frameComp1, frameComp2 = framesToCheck[index]
            frameSeg1, frameKey1 = frameComp1
            frameSeg2, frameKey2 = frameComp2
            
            frameN1 = frameSeg1[frameKey1]
            frameN2 = frameSeg2[frameKey2]

            frameN1 += frameComp1Offset
            frameN2 += frameComp2Offset

            alphaLap, diff = compare_frames(video_capture, frameN1, frameN2)

            description = "Comparing: (" + str(frameSeg1['name']) + " " + frameKey1 + " | " + str(frameSeg2['name']) + " " + frameKey2 + ")"
            description += " absDiff = " + str(diff)
            frameDesc = "RED: ( " + str(frameN1) + "  |  " + str(frameN2) + " ) :BLUE"
            cv2.putText(alphaLap, description, (50, 50),font, 0.5, (0, 255, 0),  2, cv2.LINE_AA) 
            cv2.putText(alphaLap, frameDesc , (50, 100),font, 0.5, (0, 255, 0),  2, cv2.LINE_AA) 

            debgCol = (0, 0, 0)
            debugFill = 1
            if(isEditing == 1):
                debugFill = -1
                debgCol = (0, 0, 255)
            elif(isEditing == 2):
                debugFill = -1
                debgCol = (255, 0, 0)                
            cv2.rectangle(alphaLap, (50, 125), (100, 175), thickness=debugFill, color=debgCol)

            cv2.imshow("alphaLap", alphaLap)
            key = cv2.waitKey(0) & 0xFF
            if(isEditing > 0):
                cv2.add
                
                if(isEditing == 1):
                    if key == 82:   #up
                        frameComp1Offset += 1
                    if key == 84:   #down
                        frameComp1Offset -= 1
                    if key == ord('g'):#save
                        UpdateValueByName(smData, frameSeg1['name'], frameKey1, frameN1)
                        frameComp1Offset == 0
                if(isEditing == 2):
                    if key == 82:   #up
                        frameComp2Offset += 1
                    if key == 84:   #down
                        frameComp2Offset -= 1
                    if key == ord('g'):#save
                        UpdateValueByName(smData, frameSeg2['name'], frameKey2, frameN2)
                        frameComp1Offset == 0

                
                if key == ord('r'):
                    isEditing = 1
                if key == ord('b'):
                    isEditing = 2
                if (key == 83 or key == 81):   #exitEdit
                    isEditing = 0
                    
                if key == ord('q'):
                # Write the data to a JSON file
                    with open('video_states_updated.json', 'w') as file:
                        json.dump(smData, file, indent=4)
                        break

            else:
                frameComp1Offset = 0
                frameComp2Offset = 0
                if key == 83:   #right
                    index += 1
                if key == 81:   #left
                    index -= 1 
                if key == ord('r'):
                    isEditing = 1
                if key == ord('b'):
                    isEditing = 2
                if key == ord('q'):
                    break
            
            index %= len(framesToCheck)
            
    else:
        print("NoVideo")


    # Wait for any key press to exit
    video_capture.release()
    cv2.destroyAllWindows()
    

if __name__ == "__main__":
    main()