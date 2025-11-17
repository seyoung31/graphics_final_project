## Project 5: Realtime

The project handout can be found [here](https://cs1230.graphics/projects/realtime/1).

### Output Comparison

Run the program, open the specified `.json` file and follow the instructions to set the parameters.

If the output you are generating is an image, save the image with the specified file name using the "Save Image" button in the UI. Be sure to follow the instructions in the left column to set the file name. Once you save the images, they will appear in the table below.

If the output you are generating is a video, reference the [Screen Recording Guide](https://cs1230.graphics/docs/screen-recording) to capture a video of the specified interaction. Once you've recorded everything, navigate to this template file in Github, click edit, and either select or drag-and-drop each of your videos into the correct location. This will upload them to GitHub (but not include them in the repo) and automatically embed them into this Markdown file by providing a link. Make sure to double-check that they all show up properly in the preview.

We're **not** looking for your video to **exactly** match the expected output (i.e. draw the exact same shape). Just make sure to follow the instructions and verify that the recording fully demonstrates the functionality of that part of your project.

> [!NOTE]
> There may be issues with the image saving functionality, and the images may not save the exact same way as the expected outputs. This is acceptable, as long as your output demonstrates correct functionality.

|                                           File/Method To Produce Output                                            |                                                     Expected Output                                                     |                                                                     Your Output                                                                     |
| :----------------------------------------------------------------------------------------------------------------: | :---------------------------------------------------------------------------------------------------------------------: | :-------------------------------------------------------------------------------------------------------------------------------------------------: |
|                Input: `unit_cone.json`<br/>Output: `unit_cone.png`<br/>Parameters: (5, 5, 0.1, 100)                |      ![](https://raw.githubusercontent.com/BrownCSCI1230/scenefiles/main/realtime/required_outputs/unit_cone.png)      |            ![Place unit_cone.png in student_outputs/realtime/required folder](student_outputs/realtime/required/unit_cone.png)            |
|            Input: `unit_cone_cap.json`<br/>Output: `unit_cone_cap.png`<br/>Parameters: (5, 5, 0.1, 100)            |    ![](https://raw.githubusercontent.com/BrownCSCI1230/scenefiles/main/realtime/required_outputs/unit_cone_cap.png)    |        ![Place unit_cone_cap.png in student_outputs/realtime/required folder](student_outputs/realtime/required/unit_cone_cap.png)        |
|               Input: `unit_cube.json`<br/>Output: `unit_cube.png`<br/> Parameters: (5, 5, 0.1, 100)                |      ![](https://raw.githubusercontent.com/BrownCSCI1230/scenefiles/main/realtime/required_outputs/unit_cube.png)      |            ![Place unit_cube.png in student_outputs/realtime/required folder](student_outputs/realtime/required/unit_cube.png)            |
|            Input: `unit_cylinder.json`<br/>Output: `unit_cylinder.png`<br/>Parameters: (5, 5, 0.1, 100)            |    ![](https://raw.githubusercontent.com/BrownCSCI1230/scenefiles/main/realtime/required_outputs/unit_cylinder.png)    |        ![Place unit_cylinder.png in student_outputs/realtime/required folder](student_outputs/realtime/required/unit_cylinder.png)        |
|              Input: `unit_sphere.json`<br/>Output: `unit_sphere.png`<br/>Parameters: (5, 5, 0.1, 100)              |     ![](https://raw.githubusercontent.com/BrownCSCI1230/scenefiles/main/realtime/required_outputs/unit_sphere.png)     |          ![Place unit_sphere.png in student_outputs/realtime/required folder](student_outputs/realtime/required/unit_sphere.png)          |
|          Input: `unit_cone.json`<br/>Output: `unit_cone_min.png`<br/>Parameters: (**1**, **3**, 0.1, 100)          |      ![](https://raw.githubusercontent.com/BrownCSCI1230/scenefiles/main/realtime/required_outputs/unit_cone_min.png)      |        ![Place unit_cone_min.png in student_outputs/realtime/required folder](student_outputs/realtime/required/unit_cone_min.png)        |
|      Input: `unit_cone_cap.json`<br/>Output: `unit_cone_cap_min.png`<br/>Parameters: (**1**, **3**, 0.1, 100)      |    ![](https://raw.githubusercontent.com/BrownCSCI1230/scenefiles/main/realtime/required_outputs/unit_cone_cap_min.png)    |    ![Place unit_cone_cap_min.png in student_outputs/realtime/required folder](student_outputs/realtime/required/unit_cone_cap_min.png)    |
|          Input: `unit_cube.json`<br/>Output: `unit_cube_min.png`<br/>Parameters: (**1**, **1**, 0.1, 100)          |      ![](https://raw.githubusercontent.com/BrownCSCI1230/scenefiles/main/realtime/required_outputs/unit_cube_min.png)      |        ![Place unit_cube_min.png in student_outputs/realtime/required folder](student_outputs/realtime/required/unit_cube_min.png)        |
|      Input: `unit_cylinder.json`<br/>Output: `unit_cylinder_min.png`<br/>Parameters: (**1**, **3**, 0.1, 100)      |    ![](https://raw.githubusercontent.com/BrownCSCI1230/scenefiles/main/realtime/required_outputs/unit_cylinder_min.png)    |    ![Place unit_cylinder_min.png in student_outputs/realtime/required folder](student_outputs/realtime/required/unit_cylinder_min.png)    |
|        Input: `unit_sphere.json`<br/>Output: `unit_sphere_min.png`<br/>Parameters: (**2**, **3**, 0.1, 100)        |     ![](https://raw.githubusercontent.com/BrownCSCI1230/scenefiles/main/realtime/required_outputs/unit_sphere_min.png)     |      ![Place unit_sphere_min.png in student_outputs/realtime/required folder](student_outputs/realtime/required/unit_sphere_min.png)      |
|           Input: `parse_matrix.json`<br/>Output: `parse_matrix.png`<br/>Parameters: (**3**, 5, 0.1, 100)           |    ![](https://raw.githubusercontent.com/BrownCSCI1230/scenefiles/main/realtime/required_outputs/parse_matrix.png)     |         ![Place parse_matrix.png in student_outputs/realtime/required folder](student_outputs/realtime/required/parse_matrix.png)         |
|            Input: `ambient_total.json`<br/>Output: `ambient_total.png`<br/>Parameters: (5, 5, 0.1, 100)            |    ![](https://raw.githubusercontent.com/BrownCSCI1230/scenefiles/main/realtime/required_outputs/ambient_total.png)    |        ![Place ambient_total.png in student_outputs/realtime/required folder](student_outputs/realtime/required/ambient_total.png)        |
|            Input: `diffuse_total.json`<br/>Output: `diffuse_total.png`<br/>Parameters: (5, 5, 0.1, 100)            |    ![](https://raw.githubusercontent.com/BrownCSCI1230/scenefiles/main/realtime/required_outputs/diffuse_total.png)    |        ![Place diffuse_total.png in student_outputs/realtime/required folder](student_outputs/realtime/required/diffuse_total.png)        |
|           Input: `specular_total.json`<br/>Output: `specular_total.png`<br/>Parameters: (5, 5, 0.1, 100)           |   ![](https://raw.githubusercontent.com/BrownCSCI1230/scenefiles/main/realtime/required_outputs/specular_total.png)    |       ![Place specular_total.png in student_outputs/realtime/required folder](student_outputs/realtime/required/specular_total.png)       |
|              Input: `phong_total.json`<br/>Output: `phong_total.png`<br/>Parameters: (5, 5, 0.1, 100)              |     ![](https://raw.githubusercontent.com/BrownCSCI1230/scenefiles/main/realtime/required_outputs/phong_total.png)     |          ![Place phong_total.png in student_outputs/realtime/required folder](student_outputs/realtime/required/phong_total.png)          |
|      Input: `directional_light_1.json`<br/>Output: `directional_light_1.png`<br/>Parameters: (5, 5, 0.1, 100)      | ![](https://raw.githubusercontent.com/BrownCSCI1230/scenefiles/main/realtime/required_outputs/directional_light_1.png) |  ![Place directional_light_1.png in student_outputs/realtime/required folder](student_outputs/realtime/required/directional_light_1.png)  |
| Input: `directional_light_2.json`<br/>Output: `directional_light_2.png`<br/>Parameters: (**10**, **10**, 0.1, 100) | ![](https://raw.githubusercontent.com/BrownCSCI1230/scenefiles/main/realtime/required_outputs/directional_light_2.png) |  ![Place directional_light_2.png in student_outputs/realtime/required folder](student_outputs/realtime/required/directional_light_2.png)  |
|      Input: `phong_total.json`<br/>Output: `phong_total_near_far.png`<br/>Parameters: (5, 5, **9.5**, **12**)      | ![](https://raw.githubusercontent.com/BrownCSCI1230/scenefiles/main/realtime/required_outputs/phong_total_near_far.png) | ![Place phong_total_near_far.png in student_outputs/realtime/required folder](student_outputs/realtime/required/phong_total_near_far.png) |
|      Input: `directional_light_1.json`<br/>Output: `directional_light_1_near_far.png`<br/>Parameters: (**25**, **25**, **8**, **10**)      | ![](https://raw.githubusercontent.com/BrownCSCI1230/scenefiles/main/realtime/required_outputs/directional_light_1_near_far.png) | ![Place directional_light_1_near_far.png in student_outputs/realtime/required folder](student_outputs/realtime/required/directional_light_1_near_far.png) |
| Input: `point_light_1.json`<br/>Output: `point_light_1.png`<br/>Parameters: (5, 5, 0.1, 100) | ![](https://raw.githubusercontent.com/BrownCSCI1230/scenefiles/main/realtime/required_outputs/point_light_1.png) | ![Place point_light_1.png in student_outputs/realtime/required folder](student_outputs/realtime/required/point_light_1.png) |
| Input: `point_light_2.json`<br/>Output: `point_light_2.png`<br/>Parameters: (5, 5, 0.1, 100) | ![](https://raw.githubusercontent.com/BrownCSCI1230/scenefiles/main/realtime/required_outputs/point_light_2.png) | ![Place point_light_2.png in student_outputs/realtime/required folder](student_outputs/realtime/required/point_light_2.png) |
| Input: `spot_light_1.json`<br/>Output: `spot_light_1.png`<br/> Parameters: (5, 5, 0.1, 100)  | ![](https://raw.githubusercontent.com/BrownCSCI1230/scenefiles/main/realtime/required_outputs/spot_light_1.png)  |  ![Place spot_light_1.png in student_outputs/realtime/required folder](student_outputs/realtime/required/spot_light_1.png)  |
|  Input: `spot_light_2.json`<br/>Output: `spot_light_2.png`<br/>Parameters: (5, 5, 0.1, 100)  | ![](https://raw.githubusercontent.com/BrownCSCI1230/scenefiles/main/realtime/required_outputs/spot_light_2.png)  |  ![Place spot_light_2.png in student_outputs/realtime/required folder](student_outputs/realtime/required/spot_light_2.png)  |

#### Camera Translation

_Instructions:_ Load `chess.json`. For about 1 second each in this order, press:

- <kbd>W</kbd>, <kbd>A</kbd>, <kbd>S</kbd>, <kbd>D</kbd> to move in each direction by itself
- <kbd>W+A</kbd> to move diagonally forward and to the left
- <kbd>S+D</kbd> to move diagonally backward and to the right
- <kbd>Space</kbd> to move up
- <kbd>Cmd/Ctrl</kbd> to move down

##### Expected Output

https://github.com/BrownCSCI1230/projects_realtime_template/assets/45575415/710ff8b4-6db4-445b-811d-f6c838741e67

##### Your Output

<!---
Paste your output on top of this comment!
-->

#### Camera Rotation

_Instructions:_ Load `chess.json`. Take a look around!

##### Expected Output

https://github.com/BrownCSCI1230/projects_realtime_template/assets/45575415/a14f4d32-88ee-4f5f-9843-74dd5c89b9dd

##### Your Output

<!---
Paste your output on top of this comment!
-->

### Design Choices
I made a separate shader class that handles all the uniforms being passed to the fragment shader. This felt my longest block of unique code which is why I thought it was a good candidate for a new class. 

I decided not to put the other methods into separate classes because I felt like they had repeated code that could be factored out to helper method (like initGLShape) for example. Given more time though I would definetly make another SceneHandler class to handle logic for the scene changing and maybe an initializer class too! (I'm just getting pressed up against the deadline here and want to submit).

### Collaboration/References
I used ChatGPT for some conceptual help on openGL and asked it for some debugging advice when I had a bug where I literally didn't set the far plane back far enough. 
### Known Bugs
None hopefully
### Extra Credit
why start now? 😝
