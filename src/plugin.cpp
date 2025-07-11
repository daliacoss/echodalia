#include "plugin.hpp"

rack::Plugin* pluginInstance;


void init(rack::Plugin* p) {
	pluginInstance = p;

	// Add modules here
	p->addModel(modelRonda);
	p->addModel(modelJab);

	// Any other plugin initialization may go here.
	// As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}
