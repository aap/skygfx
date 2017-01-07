![](http://i.imgur.com/RXklRRg.png)

SkyGfx (Sky is name of Renderware for the PS2) brings accurate PS2 graphics to the PC version of San Andreas, Vice City and III (and now Xbox graphics to III and Vice City).

[More information on gtaforums.](http://gtaforums.com/topic/750681-skygfx-ps2-and-xbox-graphics-for-pc/)


----------

How to compile:
Define RWSDK34 and RWSDK36 environmental variables or replace them with paths to RWSDK in premake5.lua.
For example: 

    RWSDK34: <some path>\RWSDK\RW34\include\d3d8
    RWSDK36: <some path>\RWSDK\RW36\Graphics\rwsdk\include\d3d9

Use premake to generate a solution.


