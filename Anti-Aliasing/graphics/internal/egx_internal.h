#pragma once
#include "../../io/console.h"
#include "d3dx12.h"
#include <wrl/client.h>
#include <stdexcept>

#define THROWIFFAILED(exp, error) if(FAILED(exp)) throw std::runtime_error(error)
using Microsoft::WRL::ComPtr;