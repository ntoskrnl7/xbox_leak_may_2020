vs.1.0
; shader declaration
;
; vertex stream is 0
; v0 is a float3 (position)
; v1 is a packed byte DWORD (color)
;
; .stream	0
; .loadv	v0, f3
; .loadv	v1, pb
; .end

; c[0..3] = WVP matrix

; transform vertex position

m4x4	oPos, v0, c[0]   ; r0 = vertex_pos * WVP

; copy colour
mov oD0, v1             ; r2 = diffuse colour
