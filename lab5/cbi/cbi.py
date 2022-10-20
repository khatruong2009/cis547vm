#! /usr/bin/env python3

from collections import defaultdict
import itertools
from pathlib import Path
from typing import Dict, Iterable, List, Set
from cbi.data_format import (
    CBILog,
    ObservationStatus,
    Predicate,
    PredicateInfo,
    PredicateType,
    Report,
)
from cbi.utils import get_logs


def collect_observations(log: CBILog) -> Dict[Predicate, ObservationStatus]:
    """
    Traverse the CBILog and collect observation status for each predicate.

    NOTE: If you find a `Predicate(line=3, column=5, pred_type="BranchTrue")`
    in the log, then you have observed it as True,
    further it also means you've observed is complement:
    `Predicate(line=3, column=5, pred_type="BranchFalse")` as False.

    :param log: the log
    :return: a dictionary of predicates and their observation status.
    """
    observations: Dict[Predicate, ObservationStatus] = defaultdict(
        lambda: ObservationStatus.NEVER
    )

    """
    TODO: Add your code here
    
    Hint: The PredicateType.alternatives will come in handy.
    - take every entry in the log and create predicate
    - use alternatives because each entry has multiple predicates
    - once we have tuples from the alternatives, update its status in the dictionary using ObservationStatus.merge
    """

    for i in log:
        entry = PredicateType.alternatives(i.value)
        for state, wasObserved in entry:
            predicate = Predicate(i.line, i .column, state)
            observationsPredicate = ObservationStatus.merge(
                observations[predicate], wasObserved)
            observations[predicate] = observationsPredicate

    return observations


def collect_all_predicates(logs: Iterable[CBILog]) -> Set[Predicate]:
    """
    Collect all predicates from the logs.

    :param logs: Collection of CBILogs
    :return: Set of all predicates found across all logs.
    - go through all the logs
    - go through alternatives for each and create a predicate for each and put it in a set
    - collect all the observations in the log and update the set with all the keys from the dictionary
    """
    predicates = set()

    # TODO: Add your code here
    for i in logs:
        observation = collect_observations(i)
        predicates.update(observation.keys())

    return predicates


def cbi(success_logs: List[CBILog], failure_logs: List[CBILog]) -> Report:
    """
    Compute the CBI report.

    :param success_logs: logs of successful runs
    :param failure_logs: logs of failing runs
    :return: the report
    """

    all_predicates = collect_all_predicates(
        itertools.chain(success_logs, failure_logs))

    # collect_observations(success_logs[0])

    predicate_infos: Dict[Predicate, PredicateInfo] = {
        pred: PredicateInfo(pred) for pred in all_predicates
    }

    # TODO: Add your code here to compute the information for each predicate.
    # - loop through all success logs, go through observations from each one
    # - get information from PredicateInfo

    # Finally, create a report and return it.

    # print(success_logs)
    for i in success_logs:
        observs = collect_observations(i)
        for predicate, observationStatus in observs.items():
            predicate_infos[predicate].s_obs += 1
            if (observationStatus == ObservationStatus.BOTH or observationStatus == ObservationStatus.TRUE):
                predicate_infos[predicate].s += 1

    for i in failure_logs:
        observs = collect_observations(i)
        for predicate, observationStatus in observs.items():
            predicate_infos[predicate].f_obs += 1
            if (observationStatus == ObservationStatus.BOTH or observationStatus == ObservationStatus.TRUE):
                predicate_infos[predicate].f += 1

    report = Report(predicate_info_list=list(predicate_infos.values()))
    return report
