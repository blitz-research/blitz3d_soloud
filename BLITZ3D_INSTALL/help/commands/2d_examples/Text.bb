; Text example

; enable graphics mode
Graphics 800,600,16

; wait for ESC key before ending
While Not KeyHit(1)
;print the text, centered horizontally at x=400, y=0
Font = LoadFont("C:\Windows\Fonts\Arial.ttf",16)
SetFont(Font)
Text 400,0,"Hello There!",True,False
Wend
