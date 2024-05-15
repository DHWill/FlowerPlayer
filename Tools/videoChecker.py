import cv2
import json
import time
import numpy as np



# parses the data and assigns it to a variable called jess_dict

def parseJson(file= 'video_states.json'):

    jsonFile = open(file)
    jsonFile = json.loads(jsonFile.read())
    return jsonFile


def compare_frames(video_capture, frameN1, frameN2):
    # Convert frames to grayscale
    video_capture.set(cv2.CAP_PROP_POS_FRAMES, frameN1)
    ret2, frame1 = video_capture.read()
    numpy_horizontal_concat = []
    video_capture.set(cv2.CAP_PROP_POS_FRAMES, frameN2)
    ret1, frame2 = video_capture.read()
    if(ret1 and ret2):
        gray_frame1 = cv2.cvtColor(frame1, cv2.COLOR_BGR2GRAY)
        gray_frame2 = cv2.cvtColor(frame2, cv2.COLOR_BGR2GRAY)
        diff = cv2.absdiff(gray_frame1, gray_frame2)
        diff = cv2.sumElems(diff)[0]
        red = cv2.cvtColor(gray_frame1, cv2.COLOR_GRAY2BGR)
        blue = red = cv2.cvtColor(gray_frame1, cv2.COLOR_GRAY2RGB)
        # numpy_horizontal_concat = np.concatenate((frame1, frame2), axis=1)
        overlapImage = np.zeros((gray_frame1.shape[0], gray_frame1.shape[1], 3), dtype=np.uint8)
        overlapImage[:, :, 2] = gray_frame1 #red
        overlapImage[:, :, 0] = gray_frame2 #green
        # overlap = cv2.addWeighted(red,1,blue,0.5,0)
    
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
                            frameComp1 = segCompare['startTime'] -1
                            frameComp2 = segCompareTo['startTime'] -1
                            description = "Comparing: " + segCompare['name'] + " startTime to " + segCompareTo['name'] + " startTime"
                            framesToCheck.append((frameComp1, frameComp2, description))
                        
                        if(segCompareTo['targetPosition'] == position):
                            frameComp1 = segCompare['startTime'] -1
                            frameComp2 = segment['endTime'] -1
                            description = "Comparing: " + segCompare['name'] + " startTime to " + segCompareTo['name'] + " endTime"
                            framesToCheck.append((frameComp1, frameComp2 -1, description))
                        
                if(segCompare['isEarlyExit'] == True):
                    frameComp1 = segCompare['startTime'] -1
                    frameComp2 = segCompare['transitionFromParent'] -1
                    description = "Comparing: " + segCompare['name'] + " startTime to " + segCompare['name'] + " transitionTime"
                    framesToCheck.append((frameComp1, frameComp2, description))

                        

        index = 0
        while True:
        # for frames in framesToCheck:
            frameComp1, frameComp2, description = framesToCheck[index]
            alphaLap, diff = compare_frames(video_capture, frameComp1, frameComp2)
            # printDiff = "AbsDiff between frames at {} and {} frame: {:.2f}".format(frameComp1, frameComp2, diff)
            description += " absDiff = " + str(diff)
            cv2.putText(alphaLap, description, (50, 50),font, 0.5, (0, 255, 0),  2, cv2.LINE_AA) 
            cv2.imshow("alphaLap", alphaLap)
            key = cv2.waitKey(0) & 0xFF
            if key == 83:
                index += 1
            if key == 81:
                index -= 1 
            elif key == ord('q'):
                break
            
            index %= len(framesToCheck)
            
    else:
        print("NoVideo")


    # Wait for any key press to exit
    video_capture.release()
    cv2.destroyAllWindows()
    

if __name__ == "__main__":
    main()