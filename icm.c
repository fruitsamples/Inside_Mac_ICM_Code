/*	File:		icm.c	Contains:		Written by: 		Copyright:	Copyright � 1992-1999 by Apple Computer, Inc., All Rights Reserved.				You may incorporate this Apple sample source code into your program(s) without				restriction. This Apple sample source code has been provided "AS IS" and the				responsibility for its operation is yours. You are not permitted to redistribute				this Apple sample source code as "Apple sample source code" after having made				changes. If you're going to re-distribute the source, we require that you make				it clear in the source that the code was descended from Apple sample source				code, but that you've made changes.	Change History (most recent first):				8/16/1999	Karl Groethe	Updated for Metrowerks Codewarror Pro 2.1				12/4/94		khs				changed the format of the file to the new look and feel*/// INCLUDE FILES#include "icm.h"void main(void){	WindowPtr displayWindow;	Rect windowRect;	PicHandle pic;	unsigned long stupid;	InitGraf(&qd.thePort);	InitFonts();	InitWindows();	InitMenus();	TEInit();	InitDialogs(nil);	MaxApplZone();	SetRect(&windowRect, 0, 0, 256, 256);	OffsetRect(&windowRect,						// middle of screen			   ((qd.screenBits.bounds.right - qd.screenBits.bounds.left) - windowRect.right) / 2, 				((qd.screenBits.bounds.bottom - qd.screenBits.bounds.top) - windowRect.bottom) / 2);	displayWindow = NewCWindow(nil, &windowRect, "\pImage", true, 0, (WindowPtr) - 1, true, 0);	if (displayWindow)	{		SetPort(displayWindow);		SequenceSave();		SequencePlay();	}	pic = GetQTCompressedPict((CGrafPtr)displayWindow);	EraseRect(&((CGrafPtr)displayWindow)->portRect);	Delay(30, &stupid);	if (pic)		DrawPicture(pic, &((CGrafPtr)displayWindow)->portRect);	else		DebugStr("\pGetQTCompressedPict failed");	while (!Button())		;}// FUNCTIONSvoid CheckError(OSErr error,				Str255 displayString){	if (error)	{		DebugStr(displayString);		ExitToShell();	}}PicHandle GetQTCompressedPict(CGrafPtr port){	long maxCompressedSize = 0;	Handle compressedDataH = nil;	Ptr compressedDataP;	ImageDescriptionHandle imageDescH = nil;	OSErr theErr;	PicHandle myPic = nil;	Rect bounds = port->portRect;	PixMapHandle myPixMap = port->portPixMap;	CodecType theCodecType = 'jpeg';	CodecComponent theCodec = (CodecComponent)anyCodec;	CodecQ spatialQuality = codecNormalQuality;	short depth = 0;							// let ICM choose depth	theErr = GetMaxCompressionSize(myPixMap, &bounds, depth, spatialQuality, theCodecType, 									(CompressorComponent)theCodec, &maxCompressedSize);	if (theErr)		return nil;	imageDescH = (ImageDescriptionHandle)NewHandle(4);	compressedDataH = NewHandle(maxCompressedSize);	if (compressedDataH != nil && imageDescH != nil)	{		MoveHHi(compressedDataH);		HLock(compressedDataH);		compressedDataP = StripAddress(*compressedDataH);		theErr = CompressImage(myPixMap, &bounds, spatialQuality, theCodecType, imageDescH, 								compressedDataP);		if (theErr == noErr)		{			ClipRect(&bounds);			myPic = OpenPicture(&bounds);			theErr = DecompressImage(compressedDataP, imageDescH, myPixMap, &bounds, &bounds, 										srcCopy, nil);			ClosePicture();		}		if (theErr || GetHandleSize((Handle)myPic) == sizeof(Picture))		{			KillPicture(myPic);			myPic = nil;		}	}	if (imageDescH)		DisposeHandle((Handle)imageDescH);	if (compressedDataH)		DisposeHandle(compressedDataH);	return myPic;}void MakeMyResource(StandardFileReply fileReply,					ImageDescriptionHandle description){	OSErr error;	short rfRef;	Handle sequResource;	FSpCreateResFile(&fileReply.sfFile, 'SEQM', 'SEQU', fileReply.sfScript);	error = ResError();	if (error != dupFNErr)		CheckError(error, "\pFSpCreateResFile");	rfRef = FSpOpenResFile(&fileReply.sfFile, fsRdWrPerm);	CheckError(ResError(), "\pFSpOpenResFile");	SetResLoad(false);	sequResource = Get1Resource('SEQU', 128);	if (sequResource != nil)	{		RemoveResource(sequResource);	}	SetResLoad(true);	sequResource = (Handle)description;	error = HandToHand(&sequResource);	CheckError(error, "\pHandToHand");	AddResource(sequResource, 'SEQU', 128, "\p");	CheckError(ResError(), "\pAddResource");	UpdateResFile(rfRef);	CheckError(ResError(), "\pUpdateResFile");	CloseResFile(rfRef);}void SequenceSave(void){	long filePos;	StandardFileReply fileReply;	short dfRef = 0;	OSErr error;	ImageDescriptionHandle description = nil;	StandardPutFile("\p", "\pSequence File", &fileReply);	if (fileReply.sfGood)	{		if (!(fileReply.sfReplacing))		{			error = FSpCreate(&fileReply.sfFile, 'SEQM', 'SEQU', fileReply.sfScript);			CheckError(error, "\pFSpCreate");		}		error = FSpOpenDF(&fileReply.sfFile, fsWrPerm, &dfRef);		CheckError(error, "\pFSpOpenDF");		error = SetFPos(dfRef, fsFromStart, 0);		CheckError(error, "\pSetFPos");		CompressSequence(&dfRef, &description);		error = GetFPos(dfRef, &filePos);		CheckError(error, "\pGetFPos");		error = SetEOF(dfRef, filePos);		CheckError(error, "\pSetEOF");		FlushVol(nil, fileReply.sfFile.vRefNum);		FSClose(dfRef);		dfRef = 0;		MakeMyResource(fileReply, description);		if (description != nil)			DisposeHandle((Handle)description);	}}void DrawFrame(const Rect* imageRect,			   long frameNum){	Str255 numStr;	ForeColor(redColor);	PaintRect(imageRect);	ForeColor(blueColor);	NumToString(frameNum, numStr);	MoveTo(imageRect->right / 2, imageRect->bottom / 2);	TextSize(imageRect->bottom / 3);	DrawString(numStr);}void CompressSequence(short* dfRef,					  ImageDescriptionHandle* description){	GWorldPtr currWorld = nil;	PixMapHandle currPixMap;	CGrafPtr savedPort;	GDHandle savedDevice;	Handle buffer = nil;	Ptr bufferAddr;	long compressedSize;	long dataLen;	Rect imageRect;	ImageSequence sequenceID = 0;	short frameNum;	OSErr error;	CodecType codecKind = 'rle ';	GetGWorld(&savedPort, &savedDevice);	imageRect = savedPort->portRect;	error = NewGWorld(&currWorld, 32, &imageRect, nil, nil, 0);	CheckError(error, "\pNewGWorld");	SetGWorld(currWorld, nil);	currPixMap = currWorld->portPixMap;	LockPixels(currPixMap);	//	allocate an embryonic image description structure and the	//	Image Compression Manager will resize	*description = (ImageDescriptionHandle)NewHandle(4);	error = CompressSequenceBegin(&sequenceID, currPixMap, nil,// tell ICM to allocate previous image buffer								  &imageRect, &imageRect, 0,// let ICM choose pixel depth								  codecKind, (CompressorComponent)anyCodec, codecNormalQuality,// spatial quality 								  codecNormalQuality,// temporal quality 								  5,			// at least 1 key frame every 5 frames								  nil,			// use default color table								  codecFlagUpdatePrevious, *description);	CheckError(error, "\pCompressSequenceBegin");	error = GetMaxCompressionSize(currPixMap, &imageRect, 0,// let ICM choose pixel depth								  codecNormalQuality,// spatial quality 								  codecKind, (CompressorComponent)anyCodec, &compressedSize);	CheckError(error, "\pGetMaxCompressionSize");	buffer = NewHandle(compressedSize);	CheckError(MemError(), "\pNewHandle buffer");	MoveHHi(buffer);	HLock(buffer);	bufferAddr = StripAddress(*buffer);	for (frameNum = 1; frameNum <= 10; frameNum++)	{		DrawFrame(&imageRect, frameNum);		error = CompressSequenceFrame(sequenceID, currPixMap, &imageRect, codecFlagUpdatePrevious, bufferAddr, &compressedSize, nil, nil);		CheckError(error, "\pCompressSequenceFrame");		dataLen = 4;		error = FSWrite(*dfRef, &dataLen, &compressedSize);		CheckError(error, "\pFSWrite length");		error = FSWrite(*dfRef, &compressedSize, bufferAddr);		CheckError(error, "\pFSWrite buffer");	}	error = CDSequenceEnd(sequenceID);	CheckError(error, "\pCDSequenceEnd");	DisposeGWorld(currWorld);	SetGWorld(savedPort, savedDevice);	if (buffer)		DisposeHandle(buffer);}void SequencePlay(void){	ImageDescriptionHandle description;	long compressedSize;	Handle buffer = nil;	Ptr bufferAddr;	long dataLen;	unsigned long lastTicks;	ImageSequence sequenceID;	Rect imageRect;	StandardFileReply fileReply;	SFTypeList typeList =	{		'SEQU',  0, 0, 0	};	short dfRef = 0;							// sequence data fork	short rfRef = 0;							// sequence resource fork	OSErr error;	StandardGetFile(nil, 1, typeList, &fileReply);	if (!fileReply.sfGood)		return;	rfRef = FSpOpenResFile(&fileReply.sfFile, fsRdPerm);	CheckError(ResError(), "\pFSpOpenResFile");	description = (ImageDescriptionHandle)Get1Resource('SEQU', 128);	CheckError(ResError(), "\pGet1Resource");	DetachResource((Handle)description);	HNoPurge((Handle)description);	CloseResFile(rfRef);	error = FSpOpenDF(&fileReply.sfFile, fsRdPerm, &dfRef);	CheckError(error, "\pFSpOpenDF");	buffer = NewHandle(4);	CheckError(MemError(), "\pNewHandle buffer");	SetRect(&imageRect, 0, 0, (**description).width, (**description).height);	error = DecompressSequenceBegin(&sequenceID, description, nil,// use the current port									nil,		// goto screen									&imageRect, nil,// no matrix									ditherCopy, nil,// no mask region									codecFlagUseImageBuffer, codecNormalQuality,// accuracy									(CompressorComponent)anyCodec);	while (true)	{		dataLen = 4;		error = FSRead(dfRef, &dataLen, &compressedSize);		if (error == eofErr)			break;		CheckError(error, "\pFSRead");		if (compressedSize > GetHandleSize(buffer))		{			HUnlock(buffer);			SetHandleSize(buffer, compressedSize);			CheckError(MemError(), "\pSetHandleSize");		}		HLock(buffer);		bufferAddr = StripAddress(*buffer);		error = FSRead(dfRef, &compressedSize, bufferAddr);		CheckError(error, "\pFSRead");		error = DecompressSequenceFrame(sequenceID, bufferAddr, 0,// flags										nil, nil);		CheckError(error, "\pDecompressSequenceFrame");		Delay(30, &lastTicks);	}	CDSequenceEnd(sequenceID);	FSClose(dfRef);	DisposeHandle(buffer);	DisposeHandle((Handle)description);}