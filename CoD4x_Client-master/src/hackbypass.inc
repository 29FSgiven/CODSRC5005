
; ---------------------------------------------------------------------------

cLeaf_t		struc ;	(sizeof=0x2C, align=0x4, standard type)	; XREF:	cmodel_tr
firstCollAabbIndex dw ?
collAabbCount	dw ?
brushContents	dd ?			; XREF:	sub_4EF9E0+4Bw
terrainContents	dd ?			; XREF:	sub_4EF9E0+56w
mins		dd 3 dup(?)		; XREF:	sub_4EF9E0+45w	sub_4EF9E0+50w	...
maxs		dd 3 dup(?)		; XREF:	sub_4EF9E0+6Ew	sub_4EF9E0+74w	...
leafBrushNode	dd ?			; XREF:	sub_4EF9E0+F3w
cluster		dw ?
		db ? ; undefined
		db ? ; undefined
cLeaf_t		ends

; ---------------------------------------------------------------------------

; ---------------------------------------------------------------------------

cmodel_t	struc ;	(sizeof=0x48, align=0x4, standard type)	; XREF:	clipMap_tr
mins		dd 3 dup(?)
maxs		dd 3 dup(?)
radius		dd ?
leaf		cLeaf_t	?		; XREF:	sub_4EF9E0+45w	sub_4EF9E0+4Bw	...
cmodel_t	ends

; ---------------------------------------------------------------------------

; ---------------------------------------------------------------------------

clipMap_t	struc ;	(sizeof=0x118, align=0x4) ; XREF: .data:cmr
name		dd ?			; XREF:	sub_488C60r sub_4ED8F0r ... ;	offset
planeCount	dd ?			; XREF:	sub_4EF200+58w	sub_602060+153r
planes		dd ?			; XREF:	sub_4EEA80+6Ar	sub_4EEB40+24Dr ... ; offset
numStaticModels	dd ?			; XREF:	sub_4F2830+2Aw	sub_4F2830+353r ...
staticModelList	dd ?			; XREF:	sub_4F2830+30w	sub_4F2830+128w ... ; offset
numMaterials	dd ?			; XREF:	sub_4EDA10+50w	sub_4EDA10+60r	...
materials	dd ?			; XREF:	sub_4EDA10+4Bw
					; sub_4EDA10:loc_4EDA80r ... ;	offset
numBrushSides	dd ?			; XREF:	sub_4EEB40+EDw
brushsides	dd ?			; XREF:	sub_4EEB40+E8w	; offset
numBrushEdges	dd ?			; XREF:	sub_4EEB40+94w
brushEdges	dd ?			; XREF:	sub_4EEB40+8Fw	sub_4EEB40+A3r	; offset
numNodes	dd ?			; XREF:	sub_4EEA80+50w
nodes		dd ?			; XREF:	sub_4EEA80+4Bw	sub_4F2BF0+4r ... ; offset
numLeafs	dd ?			; XREF:	sub_4EEE60+48w	sub_4EEF30+48w	...
leafs		dd ?			; XREF:	sub_4EEE60+43w	sub_4EEF30+43w	... ; offset
leafbrushNodesCount dd ?		; XREF:	sub_4EF7A0+A9w
leafbrushNodes	dd ?			; XREF:	sub_4EE770+163r sub_4EF7A0+5Aw ... ; offset
numLeafBrushes	dd ?			; XREF:	sub_4EF310+31w	sub_4EF9E0+FEr	...
leafbrushes	dd ?			; XREF:	sub_4EF000+53r	sub_4EF000+B9r	... ; offset
numLeafSurfaces	dd ?			; XREF:	sub_4EF890+9Fw
leafsurfaces	dd ?			; XREF:	sub_4EF890+9Aw	; offset
vertCount	dd ?			; XREF:	sub_4EF890+D8w
verts		dd ?			; XREF:	sub_4EF890+D3w	sub_4EFCE0+Er ... ; offset
triCount	dd ?			; XREF:	sub_4EF380+3Aw
triIndices	dd ?			; XREF:	sub_4EF380+34w	sub_4F2000+B4r	... ; offset
triEdgeIsWalkable dd ?			; XREF:	sub_4EF890+10Dw sub_4EFCB0+11r ; offset
borderCount	dd ?			; XREF:	sub_4EF3D0+39w
borders		dd ?			; XREF:	sub_4EF3D0+34w	sub_4EF450+5Er	; offset
partitionCount	dd ?			; XREF:	sub_4ED880r sub_4EF450+33w
partitions	dd ?			; XREF:	sub_4EF450+2Ew	sub_4F2000+86r	... ; offset
aabbTreeCount	dd ?			; XREF:	sub_4EF4D0+31w
aabbTrees	dd ?			; XREF:	sub_4EE940+C8r	sub_4EF000+87r	... ; offset
numSubModels	dd ?			; XREF:	sub_459080+12r
					; CM_ModelBounds+4r ...
cmodels		dd ?			; XREF:	sub_459080+1Ar	sub_490400+69r	... ; offset
numBrushes	dw ?			; XREF:	sub_4EEB40+109w sub_4EF9E0r ...
		db ? ; undefined
		db ? ; undefined
brushes		dd ?			; XREF:	sub_4EDC60+18r	sub_4EDF90+87r	... ; offset
numClusters	dd ?			; XREF:	sub_4EEE60:loc_4EEF08r
					; sub_4EEE60+B3w ...
clusterBytes	dd ?			; XREF:	sub_4EF700+34w	sub_4EF700+44r	...
visibility	dd ?			; XREF:	sub_4EF700+51w	sub_4EF700+8Dw	... ; offset
vised		dd ?			; XREF:	sub_4EF700:loc_4EF762w
					; sub_4F3010r ...
mapEnts		dd ?			; XREF:	G_SpawnEntitiesFromString+43r
					; G_SpawnEntitiesFromString:loc_4E09E6r ... ; offset
box_brush	dd ?			; XREF:	sub_4ED880+2Dr	sub_4EF9E0+1Bw	... ; offset
box_model	cmodel_t ?		; XREF:	sub_4ED880+63o	sub_4EF9E0+45w	...
dynEntCount	dw 2 dup(?)		; XREF:	sub_48D000+17r	sub_48D110+12r	...
dynEntDefList	dd 2 dup(?)		; XREF:	sub_48C9A0+Er sub_48CF20+16r ... ; offset
dynEntPoseList	dd 2 dup(?)		; XREF:	sub_48C9A0+2Ar	sub_48CF20+1Fr	... ; offset
dynEntClientList dd 2 dup(?)		; XREF:	sub_48D000+47r
					; sub_48D110:loc_48D140r ... ;	offset
dynEntCollList	dd 2 dup(?)		; XREF:	sub_48D000+4Er
					; sub_48D3F0:loc_48D474r ... ;	offset
checksum	dd ?			; XREF:	CM_LoadMap+5Ar
clipMap_t	ends

; ---------------------------------------------------------------------------


