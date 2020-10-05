#include "sampler.h"
#include "internal/egx_internal.h"

void egx::Sampler::SetVisibility(ShaderVisibility visibility)
{
	desc.ShaderVisibility = convertVisibility(visibility);
}