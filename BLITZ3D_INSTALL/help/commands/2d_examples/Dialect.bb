; Dialect Example
; ---------------

Dialect "modern"

Type Block
	Field id
End Type

; In modern dialect, use ':' to declare object type instead of '.'
block:Block = New Block

; In modern dialect, use '.' to access fields instead of '\'
block.id=100
