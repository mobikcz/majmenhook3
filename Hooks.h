/*
Syn's AyyWare Framework 2015
*/

#pragma once

// It's actually in DLLMain but w/e
extern bool DoUnload;
//extern bool bGlovesNeedUpdate;

#include "Utilities.h"
namespace Hooks
{
	void Initialise();
	void UndoHooks();

	// VMT Managers
	extern Utilities::Memory::VMTManager VMTPanel; // Hooking drawing functions
	extern Utilities::Memory::VMTManager VMTClient; // Maybe CreateMove
	extern Utilities::Memory::VMTManager VMTClientMode; // CreateMove for functionality
	extern Utilities::Memory::VMTManager VMTModelRender; // DrawModelEx for chams
	extern Utilities::Memory::VMTManager VMTPrediction; // InPrediction for no vis recoil
	extern Utilities::Memory::VMTManager VMTPlaySound; // Autoaccept shit
	extern Utilities::Memory::VMTManager VMTRenderView;
	extern Utilities::Memory::VMTManager VMTMaterialSystem;
};

namespace Resolver
{
	extern bool didhitHS;
}

extern bool flipAA;
extern bool flipAA;
extern float lineRealAngle;
extern float lineFakeAngle;
extern float headPos;
extern float flipBool;
extern int predicting;

