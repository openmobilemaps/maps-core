//
// Created by Christoph Maurhofer on 08.02.2021.
//

#pragma once

class Tiled2dMapSourceListenerInterface {
  public:
    virtual void onTilesUpdated() = 0;
};