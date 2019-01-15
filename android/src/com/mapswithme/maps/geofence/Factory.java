package com.mapswithme.maps.geofence;

import android.support.annotation.NonNull;

import com.google.android.gms.location.Geofence;
import com.mapswithme.maps.bookmarks.data.FeatureId;

class Factory
{
  @NonNull
  public static FeatureId from(@NonNull Geofence geofence)
  {
    String requestId = geofence.getRequestId();
    return FeatureId.fromString(requestId);
  }
}
