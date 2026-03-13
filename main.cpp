#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <algorithm>
#include <sstream>

using namespace std;

struct Submission {
    char problem;
    string status;
    int time;
    bool isFrozen;
};

struct ProblemStatus {
    bool solved = false;
    int firstSolveTime = 0;
    int wrongAttemptsBefore = 0;
    vector<int> frozenSubmissionIndices;
};

struct Team {
    string name;
    ProblemStatus problems[26];
    vector<Submission> submissions;
    int solvedCount = 0;
    int penaltyTime = 0;
    vector<int> solveTimes;
    int rank = 0;

    void addSolveTime(int time) {
        solveTimes.insert(lower_bound(solveTimes.begin(), solveTimes.end(), time), time);
    }
};

bool compareTeams(const Team& t1, const Team& t2) {
    if (t1.solvedCount != t2.solvedCount) {
        return t1.solvedCount > t2.solvedCount;
    }
    if (t1.penaltyTime != t2.penaltyTime) {
        return t1.penaltyTime < t2.penaltyTime;
    }

    int s1 = t1.solveTimes.size();
    int s2 = t2.solveTimes.size();
    for (int i = s1 - 1, j = s2 - 1; i >= 0 && j >= 0; i--, j--) {
        if (t1.solveTimes[i] != t2.solveTimes[j]) {
            return t1.solveTimes[i] < t2.solveTimes[j];
        }
    }

    return t1.name < t2.name;
}

class ICPCSystem {
private:
    unordered_map<string, Team> teams;
    vector<string> teamOrder;
    bool started = false;
    int durationTime = 0;
    int problemCount = 0;
    bool frozen = false;
    bool hasFlushed = false;

    void calculateRankings() {
        vector<string> names = teamOrder;
        sort(names.begin(), names.end(), [this](const string& a, const string& b) {
            return compareTeams(teams[a], teams[b]);
        });

        for (size_t i = 0; i < names.size(); i++) {
            teams[names[i]].rank = i + 1;
        }
    }

    string getProblemStatusStr(const Team& team, int probIdx) {
        const ProblemStatus& ps = team.problems[probIdx];

        if (ps.solved) {
            if (ps.wrongAttemptsBefore == 0) {
                return "+";
            } else {
                return "+" + to_string(ps.wrongAttemptsBefore);
            }
        } else if (!ps.frozenSubmissionIndices.empty()) {
            int frozenCount = ps.frozenSubmissionIndices.size();
            if (ps.wrongAttemptsBefore == 0) {
                return "0/" + to_string(frozenCount);
            } else {
                return "-" + to_string(ps.wrongAttemptsBefore) + "/" + to_string(frozenCount);
            }
        } else {
            if (ps.wrongAttemptsBefore == 0) {
                return ".";
            } else {
                return "-" + to_string(ps.wrongAttemptsBefore);
            }
        }
    }

    void printScoreboard() {
        for (const string& name : teamOrder) {
            const Team& team = teams[name];
            cout << team.name << " " << team.rank << " " << team.solvedCount
                 << " " << team.penaltyTime;

            for (int i = 0; i < problemCount; i++) {
                cout << " " << getProblemStatusStr(team, i);
            }
            cout << "\n";
        }
    }

public:
    void addTeam(const string& name) {
        if (started) {
            cout << "[Error]Add failed: competition has started.\n";
        } else if (teams.find(name) != teams.end()) {
            cout << "[Error]Add failed: duplicated team name.\n";
        } else {
            Team t;
            t.name = name;
            teams[name] = t;
            teamOrder.push_back(name);
            cout << "[Info]Add successfully.\n";
        }
    }

    void startCompetition(int duration, int problems) {
        if (started) {
            cout << "[Error]Start failed: competition has started.\n";
        } else {
            started = true;
            durationTime = duration;
            problemCount = problems;
            cout << "[Info]Competition starts.\n";
        }
    }

    void submit(const string& problem, const string& teamName,
                const string& status, int time) {
        Team& team = teams[teamName];
        int probIdx = problem[0] - 'A';

        Submission sub;
        sub.problem = problem[0];
        sub.status = status;
        sub.time = time;
        sub.isFrozen = false;

        int subIdx = team.submissions.size();
        team.submissions.push_back(sub);

        ProblemStatus& ps = team.problems[probIdx];
        bool wasSolvedBefore = ps.solved;

        if (!frozen || wasSolvedBefore) {
            if (status == "Accepted" && !ps.solved) {
                ps.solved = true;
                ps.firstSolveTime = time;
                team.solvedCount++;
                team.penaltyTime += 20 * ps.wrongAttemptsBefore + time;
                team.addSolveTime(time);
            } else if (status != "Accepted" && !ps.solved) {
                ps.wrongAttemptsBefore++;
            }
        } else {
            team.submissions[subIdx].isFrozen = true;
            ps.frozenSubmissionIndices.push_back(subIdx);
        }
    }

    void flush() {
        calculateRankings();
        hasFlushed = true;
        cout << "[Info]Flush scoreboard.\n";
    }

    void freeze() {
        if (frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
        } else {
            frozen = true;
            cout << "[Info]Freeze scoreboard.\n";
        }
    }

    void scroll() {
        if (!frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            return;
        }

        cout << "[Info]Scroll scoreboard.\n";

        calculateRankings();
        printScoreboard();

        while (true) {
            string lowestTeam = "";
            int lowestRank = -1;

            for (const string& name : teamOrder) {
                Team& team = teams[name];
                if (team.rank <= lowestRank) continue;

                bool hasFrozen = false;
                for (int i = 0; i < problemCount; i++) {
                    if (!team.problems[i].frozenSubmissionIndices.empty()) {
                        hasFrozen = true;
                        break;
                    }
                }

                if (hasFrozen) {
                    lowestRank = team.rank;
                    lowestTeam = name;
                }
            }

            if (lowestTeam.empty()) break;

            Team& team = teams[lowestTeam];

            int minFrozenProbIdx = -1;
            for (int i = 0; i < problemCount; i++) {
                if (!team.problems[i].frozenSubmissionIndices.empty()) {
                    minFrozenProbIdx = i;
                    break;
                }
            }

            ProblemStatus& ps = team.problems[minFrozenProbIdx];
            int oldRank = team.rank;

            for (int idx : ps.frozenSubmissionIndices) {
                const Submission& sub = team.submissions[idx];
                if (sub.status == "Accepted" && !ps.solved) {
                    ps.solved = true;
                    ps.firstSolveTime = sub.time;
                    team.solvedCount++;
                    team.penaltyTime += 20 * ps.wrongAttemptsBefore + sub.time;
                    team.addSolveTime(sub.time);
                } else if (sub.status != "Accepted" && !ps.solved) {
                    ps.wrongAttemptsBefore++;
                }
            }

            ps.frozenSubmissionIndices.clear();

            calculateRankings();

            if (team.rank < oldRank) {
                string replaced = "";
                for (const string& n : teamOrder) {
                    if (teams[n].rank == oldRank && n != lowestTeam) {
                        replaced = n;
                        break;
                    }
                }
                cout << team.name << " " << replaced << " "
                     << team.solvedCount << " " << team.penaltyTime << "\n";
            }
        }

        printScoreboard();
        frozen = false;
    }

    void queryRanking(const string& name) {
        if (teams.find(name) == teams.end()) {
            cout << "[Error]Query ranking failed: cannot find the team.\n";
        } else {
            cout << "[Info]Complete query ranking.\n";
            if (frozen) {
                cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
            }
            if (!hasFlushed) {
                vector<string> sorted = teamOrder;
                sort(sorted.begin(), sorted.end());
                int rank = 1;
                for (const string& n : sorted) {
                    if (n == name) break;
                    rank++;
                }
                cout << name << " NOW AT RANKING " << rank << "\n";
            } else {
                cout << name << " NOW AT RANKING " << teams[name].rank << "\n";
            }
        }
    }

    void querySubmission(const string& name, const string& problem,
                         const string& status) {
        if (teams.find(name) == teams.end()) {
            cout << "[Error]Query submission failed: cannot find the team.\n";
        } else {
            cout << "[Info]Complete query submission.\n";

            const Team& team = teams[name];
            Submission found;
            bool foundAny = false;

            for (int i = (int)team.submissions.size() - 1; i >= 0; i--) {
                const Submission& sub = team.submissions[i];
                bool problemMatch = (problem == "ALL" || sub.problem == problem[0]);
                bool statusMatch = (status == "ALL" || sub.status == status);

                if (problemMatch && statusMatch) {
                    found = sub;
                    foundAny = true;
                    break;
                }
            }

            if (foundAny) {
                cout << name << " " << found.problem << " "
                     << found.status << " " << found.time << "\n";
            } else {
                cout << "Cannot find any submission.\n";
            }
        }
    }

    void end() {
        cout << "[Info]Competition ends.\n";
    }
};

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(nullptr);

    ICPCSystem system;
    string line;

    while (getline(cin, line)) {
        if (line.empty()) continue;

        istringstream iss(line);
        string cmd;
        iss >> cmd;

        if (cmd == "ADDTEAM") {
            string name;
            iss >> name;
            system.addTeam(name);
        } else if (cmd == "START") {
            string duration, problem;
            int dur, prob;
            iss >> duration >> dur >> problem >> prob;
            system.startCompetition(dur, prob);
        } else if (cmd == "SUBMIT") {
            string problem, by, teamName, with, status, at;
            int time;
            iss >> problem >> by >> teamName >> with >> status >> at >> time;
            system.submit(problem, teamName, status, time);
        } else if (cmd == "FLUSH") {
            system.flush();
        } else if (cmd == "FREEZE") {
            system.freeze();
        } else if (cmd == "SCROLL") {
            system.scroll();
        } else if (cmd == "QUERY_RANKING") {
            string name;
            iss >> name;
            system.queryRanking(name);
        } else if (cmd == "QUERY_SUBMISSION") {
            string teamName, where, rest;
            iss >> teamName >> where;
            getline(iss, rest);

            size_t probPos = rest.find("PROBLEM=");
            size_t statusPos = rest.find("STATUS=");

            string problem, status;
            if (probPos != string::npos) {
                size_t start = probPos + 8;
                size_t end = rest.find(" AND", start);
                problem = rest.substr(start, end - start);
            }
            if (statusPos != string::npos) {
                size_t start = statusPos + 7;
                status = rest.substr(start);
            }

            system.querySubmission(teamName, problem, status);
        } else if (cmd == "END") {
            system.end();
            break;
        }
    }

    return 0;
}
