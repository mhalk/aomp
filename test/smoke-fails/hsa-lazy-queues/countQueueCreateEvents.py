#!/usr/bin/env python3

from io import StringIO
from collections import OrderedDict
import json
import sys
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('num_occurrences', type=int, help='How many hsa_queue_create events are expected')
args = parser.parse_args()

DataFilename = 'results.json'

hsa_queue_create_name = 'hsa_queue_create'

# Idea: We can store this info in another file (e.g. for other tests)
EventsOfInterest = {
  # Events of kind 'traceEvents' where key 'name' is prefixed with
  # 'hsa_queue_create' or '__omp_offloading_'
  "traceEvents": [
    {
      # 'key' and a list of properties to crawl
      "name": [
        { "id": 'hsa_queue_create',  "alias": "Q", "expectedCount": args.num_occurrences },
        { "id": '__omp_offloading_', "alias": "K", "expectedCount": 67 }
      ]
    }
  ]
}


def loadEvents(EventsOfInterest):
  # The returned dict holding the events of interest
  Events = {}
  with open(DataFilename, "r") as f:
    AllEvents = json.load(f)

  # Iterate all interesting 'kinds'
  # Since we want to update the EventsOfInterest dict not very often, we select
  # the kind upfront. Thus, we have to iterate over the results multiple times.
  for kind in EventsOfInterest:
    # Select the corresponding results
    AllEventsOfKind = AllEvents[kind]
    # For each of the requested keys (stored in a list)
    for eventKeys in EventsOfInterest[kind]:
      # And their respective key and requested properties
      for key, properties in eventKeys.items():
        for p in properties:
          EventCount = 0
          # Finally iterate over all events
          for e in AllEventsOfKind:
            # Check if key exists and it's value
            if key in e and e[key].startswith(p['id']):
              EventCount = EventCount + 1
              EventId = p['alias'] + str(EventCount)
              Events[EventId] = e

          # Update the corresponding EventCount in EventsOfInterest dict
          p.update({'count': EventCount})

  #for k, v in Events.items():
    #print(k)
  #print(EventsOfInterest)

  return Events


def createTimestampList(Events):
  Timestamps = {}

  # Add all 'begin' and 'end' timestamps, we know they are stored in 'args' as
  # 'BeginNs' / 'EndNs'
  # But: Watch out for possible duplicates
  for k, v in Events.items():
    # Load the timestamps as integer, which will be used as key
    begin = int(v['args']['BeginNs'])
    end = int(v['args']['EndNs'])

    if not begin in Timestamps:
      Timestamps[begin] = [{k: 1}]
      print("Adding (1) item: {}: {}".format(begin, k))
    else:
      Timestamps[begin].append({k: 1})
      print("Apping (1) item: {}: {}".format(begin, k))

    if not end in Timestamps:
      Timestamps[end] = [{k: 0}]
      print("Adding (0) item: {}: {}".format(end, k))
    else:
      Timestamps[end].append({k: 0})
      print("Apping (0) item: {}: {}".format(end, k))

  # Create a version of the current dict that is sorted by timestamp / key
  Timestamps = dict(sorted(Timestamps.items()))

  previousTimestamp = 0
  for timestamp, eventList in Timestamps.items():
    print("{}: {} \t({:9d})".format(timestamp, eventList,
                                    timestamp - previousTimestamp))
    previousTimestamp = timestamp

  return Timestamps


def createTimelineLog(Timestamps):
  Log = StringIO("")
  LogLine = 0
  ActiveSet = {}
  Activations = 0

  # Get the very first timestamp from the ordered dict
  OffsetTimestamp = next(iter(Timestamps))
  CurrentTimestamp = 0.

  State = 0
  for t, events in Timestamps.items():
    # Load the events (list of dicts)
    for e in events:
      # Get the dict entry key (= event's name, like 'Q2' or 'K7')
      EventName = next(iter(e))

      # Check if timestamps and states differ
      if (CurrentTimestamp < t or State != e[EventName]):
        CurrentTimestamp = t
        for a in ActiveSet:
          Log.write(" {:4s}".format(a))
        Log.write("\n{:6d} {:15d}".format(LogLine, (CurrentTimestamp - OffsetTimestamp)))
        LogLine += 1

      # Set current event's state
      State = e[EventName]
      if State == 1:
        ActiveSet[EventName] = 1
        Activations += 1
      else:
        del ActiveSet[EventName]

      # print(EventName, "=", e[EventName])
      e.items()
      Log.write("")


  print(Log.getvalue())
  return Log.getvalue()


def searchAndCount(TheJSON, args) -> None:
  TraceEvents = TheJSON['traceEvents']

  ExpectedNumOccurs = args.num_occurrences
  NumOccurs = 0
  for e in TraceEvents:
    if len(e) == 0:
      continue

    if e['name'] == hsa_queue_create_name:
      NumOccurs += 1
      #print("Encountered {}".format(hsa_queue_create_name))
    #elif e['name'].startswith(kernel_launch_name_prefix):
      #print("Encountered {}".format(e['name']))

  # if NumOccurs != ExpectedNumOccurs:
  #    sys.exit(1)

def print_ts(TheJSON, args) -> None:
  TraceEvents = TheJSON['traceEvents']
  t0 = -1
  violations = 0
  for e in TraceEvents:
    if 'pid' in e:
      if t0 > int(e['pid']):
        violations = violations + 1
        print("  NOK  -- {:48s} \t({:5d})".format(e['name'], int(e['pid'])-t0))

      t0 = int(e['pid'])

  print("Found {} violations".format(violations))

if __name__ == '__main__':
  Events = loadEvents(EventsOfInterest)
  Timestamps = createTimestampList(Events)
  createTimelineLog(Timestamps)
  print(EventsOfInterest)

  # args = parser.parse_args()
  with open(DataFilename, "r") as f:
    J = json.load(f)
    searchAndCount(J, args)
