//
// author Kazys Stepanas
//
// Copyright (c) Kazys Stepanas 2023

// This file ensures we can parse ServerApi.h with TES_ENABLE undefined.
#ifdef TES_ENABLE
#undef TES_ENABLE
#endif  // TES_ENABLE

#include "ServerApi.h"
