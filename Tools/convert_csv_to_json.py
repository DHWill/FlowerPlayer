import csv
import json

def csv_to_json(sm_file, json_file):
    sm_data = []
    output_data = []
    
    # Read the CSV file
    with open(sm_file, 'r') as file1:
        sm_reader = csv.DictReader(file1)
        for row in sm_reader:
            sm_data.append(row)
    
    for sm_anim in sm_data:
        clipName = sm_anim['Clip Name']
        startTime = int(sm_anim['From'])
        endTime = int(sm_anim['To'])
        parentAnimation = sm_anim['Parents']
        transitionFrame = 0
        isEarlyExit = False
        if(parentAnimation != ''):
            isEarlyExit = True
            transitionFrame = int(sm_anim['Transition Frame'])
        if(sm_anim['IsActive'] == "TRUE"):
            isActive = True
        else: 
            isActive = False
        
        positions = {"PB":0, "BB":1, "RG":2}
        position = sm_anim['State']
        state = sm_anim['State']
        position = positions.get(sm_anim['State'])
        targetPosition = positions.get(sm_anim['NextState'])
        output_data.append({'name':clipName, 
                            'startTime':startTime, 
                            'endTime':endTime,
                            'transitionFromParent': transitionFrame,
                            'parentAnimation' : parentAnimation,
                            'parentState':state,
                            'position':position,
                            'targetPosition':targetPosition,
                            'isEarlyExit':isEarlyExit,
                            'isActive':isActive})
    # Write the data to a JSON file
    with open(json_file, 'w') as file:
        json.dump(output_data, file, indent=4)
        
        




    
        
    
# Replace 'inputcsv' and 'output.json' with the actual file names


csv_to_json('SM.csv', 'video_states.json')
