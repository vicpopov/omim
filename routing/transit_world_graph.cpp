#include "routing/transit_world_graph.hpp"

#include "routing/index_graph.hpp"
#include "routing/transit_graph.hpp"

#include <memory>
#include <utility>

namespace routing
{
using namespace std;

TransitWorldGraph::TransitWorldGraph(unique_ptr<CrossMwmGraph> crossMwmGraph,
                                     unique_ptr<IndexGraphLoader> indexLoader,
                                     unique_ptr<TransitGraphLoader> transitLoader,
                                     shared_ptr<EdgeEstimator> estimator)
  : m_crossMwmGraph(move(crossMwmGraph))
  , m_indexLoader(move(indexLoader))
  , m_transitLoader(move(transitLoader))
  , m_estimator(estimator)
{
  CHECK(m_indexLoader, ());
  CHECK(m_transitLoader, ());
  CHECK(m_estimator, ());
}

void TransitWorldGraph::GetEdgeList(Segment const & segment, bool isOutgoing, bool useRoutingOptions,
                                    vector<SegmentEdge> & edges)
{
  auto & transitGraph = GetTransitGraph(segment.GetMwmId());

  if (TransitGraph::IsTransitSegment(segment))
  {
    transitGraph.GetTransitEdges(segment, isOutgoing, edges);

    Segment real;
    if (transitGraph.FindReal(segment, real))
    {
      bool const haveSameFront = GetJunction(segment, true /* front */) == GetJunction(real, true);
      bool const haveSameBack = GetJunction(segment, false /* front */) == GetJunction(real, false);
      if ((isOutgoing && haveSameFront) || (!isOutgoing && haveSameBack))
        AddRealEdges(real, isOutgoing, useRoutingOptions, edges);
    }

    GetTwins(segment, isOutgoing, useRoutingOptions, edges);
  }
  else
  {
    AddRealEdges(segment, isOutgoing, useRoutingOptions, edges);
  }

  vector<SegmentEdge> fakeFromReal;
  for (auto const & edge : edges)
  {
    auto const & edgeSegment = edge.GetTarget();
    for (auto const & s : transitGraph.GetFake(edgeSegment))
    {
      bool const haveSameFront = GetJunction(edgeSegment, true /* front */) == GetJunction(s, true);
      bool const haveSameBack = GetJunction(edgeSegment, false /* front */) == GetJunction(s, false);
      if ((isOutgoing && haveSameBack) || (!isOutgoing && haveSameFront))
        fakeFromReal.emplace_back(s, edge.GetWeight());
    }
  }
  edges.insert(edges.end(), fakeFromReal.begin(), fakeFromReal.end());
}

void TransitWorldGraph::GetEdgeList(JointSegment const & parentJoint,
                                    Segment const & segment, bool isOutgoing,
                                    std::vector<JointEdge> & edges,
                                    std::vector<RouteWeight> & parentWeights)
{
  CHECK(false, ("TransitWorldGraph does not support Joints mode."));
}

geometry::PointWithAltitude const & TransitWorldGraph::GetJunction(Segment const & segment,
                                                                   bool front)
{
  if (TransitGraph::IsTransitSegment(segment))
    return GetTransitGraph(segment.GetMwmId()).GetJunction(segment, front);

  return GetRealRoadGeometry(segment.GetMwmId(), segment.GetFeatureId())
      .GetJunction(segment.GetPointId(front));
}

m2::PointD const & TransitWorldGraph::GetPoint(Segment const & segment, bool front)
{
  return GetJunction(segment, front).GetPoint();
}

bool TransitWorldGraph::IsOneWay(NumMwmId mwmId, uint32_t featureId)
{
  if (TransitGraph::IsTransitFeature(featureId))
    return true;
  return GetRealRoadGeometry(mwmId, featureId).IsOneWay();
}

bool TransitWorldGraph::IsPassThroughAllowed(NumMwmId mwmId, uint32_t featureId)
{
  if (TransitGraph::IsTransitFeature(featureId))
    return true;
  return GetRealRoadGeometry(mwmId, featureId).IsPassThroughAllowed();
}

void TransitWorldGraph::ClearCachedGraphs()
{
  m_indexLoader->Clear();
  m_transitLoader->Clear();
}

RouteWeight TransitWorldGraph::HeuristicCostEstimate(Segment const & from, Segment const & to)
{
  return HeuristicCostEstimate(GetPoint(from, true /* front */), GetPoint(to, true /* front */));
}

RouteWeight TransitWorldGraph::HeuristicCostEstimate(Segment const & from, m2::PointD const & to)
{
  return HeuristicCostEstimate(GetPoint(from, true /* front */), to);
}

RouteWeight TransitWorldGraph::HeuristicCostEstimate(m2::PointD const & from, m2::PointD const & to)
{
  return RouteWeight(m_estimator->CalcHeuristic(from, to));
}

RouteWeight TransitWorldGraph::CalcSegmentWeight(Segment const & segment,
                                                 EdgeEstimator::Purpose purpose)
{
  if (TransitGraph::IsTransitSegment(segment))
  {
    TransitGraph & transitGraph = GetTransitGraph(segment.GetMwmId());
    return transitGraph.CalcSegmentWeight(segment, purpose);
  }

  return RouteWeight(m_estimator->CalcSegmentWeight(
      segment, GetRealRoadGeometry(segment.GetMwmId(), segment.GetFeatureId()), purpose));
}

RouteWeight TransitWorldGraph::CalcLeapWeight(m2::PointD const & from, m2::PointD const & to) const
{
  return RouteWeight(m_estimator->CalcLeapWeight(from, to));
}

RouteWeight TransitWorldGraph::CalcOffroadWeight(m2::PointD const & from,
                                                 m2::PointD const & to,
                                                 EdgeEstimator::Purpose purpose) const
{
  return RouteWeight(m_estimator->CalcOffroad(from, to, purpose));
}

double TransitWorldGraph::CalculateETA(Segment const & from, Segment const & to)
{
  if (TransitGraph::IsTransitSegment(from))
    return CalcSegmentWeight(to, EdgeEstimator::Purpose::ETA).GetWeight();

  if (TransitGraph::IsTransitSegment(to))
    return CalcSegmentWeight(to, EdgeEstimator::Purpose::ETA).GetWeight();

  if (from.GetMwmId() != to.GetMwmId())
  {
    return m_estimator->CalcSegmentWeight(to, GetRealRoadGeometry(to.GetMwmId(), to
        .GetFeatureId()), EdgeEstimator::Purpose::ETA);
  }

  auto & indexGraph = m_indexLoader->GetIndexGraph(from.GetMwmId());
  return indexGraph
      .CalculateEdgeWeight(EdgeEstimator::Purpose::ETA, true /* isOutgoing */, from, to)
      .GetWeight();
}

double TransitWorldGraph::CalculateETAWithoutPenalty(Segment const & segment)
{
  if (TransitGraph::IsTransitSegment(segment))
    return CalcSegmentWeight(segment, EdgeEstimator::Purpose::ETA).GetWeight();

  return m_estimator->CalcSegmentWeight(
      segment, GetRealRoadGeometry(segment.GetMwmId(), segment.GetFeatureId()),
      EdgeEstimator::Purpose::ETA);
}

unique_ptr<TransitInfo> TransitWorldGraph::GetTransitInfo(Segment const & segment)
{
  if (!TransitGraph::IsTransitSegment(segment))
    return {};

  auto & transitGraph = GetTransitGraph(segment.GetMwmId());
  if (transitGraph.IsGate(segment))
    return make_unique<TransitInfo>(transitGraph.GetGate(segment));

  if (transitGraph.IsEdge(segment))
    return make_unique<TransitInfo>(transitGraph.GetEdge(segment));

  // Fake segment between pedestrian feature and gate.
  return {};
}

void TransitWorldGraph::GetTwinsInner(Segment const & segment, bool isOutgoing,
                                      vector<Segment> & twins)
{
  if (m_mode == WorldGraphMode::SingleMwm || !m_crossMwmGraph ||
      !m_crossMwmGraph->IsTransition(segment, isOutgoing))
  {
    return;
  }
  m_crossMwmGraph->GetTwins(segment, isOutgoing, twins);
}

RoadGeometry const & TransitWorldGraph::GetRealRoadGeometry(NumMwmId mwmId, uint32_t featureId)
{
  CHECK(!TransitGraph::IsTransitFeature(featureId), ("GetRealRoadGeometry not designed for transit."));
  return m_indexLoader->GetGeometry(mwmId).GetRoad(featureId);
}

void TransitWorldGraph::AddRealEdges(Segment const & segment, bool isOutgoing,
                                     bool useRoutingOptions, vector<SegmentEdge> & edges)
{
  auto & indexGraph = GetIndexGraph(segment.GetMwmId());
  indexGraph.GetEdgeList(segment, isOutgoing, useRoutingOptions, edges);
  GetTwins(segment, isOutgoing, useRoutingOptions, edges);
}

TransitGraph & TransitWorldGraph::GetTransitGraph(NumMwmId mwmId)
{
  auto & indexGraph = GetIndexGraph(mwmId);
  return m_transitLoader->GetTransitGraph(mwmId, indexGraph);
}
}  // namespace routing
