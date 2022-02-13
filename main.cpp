#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

typedef uint8_t dbByte;   // 8 Bits (1 Byte)
typedef uint16_t dbShort; // 16 Bits (2 Bytes)
typedef uint32_t dbInt;   // 32 Bits (4 Bytes)
typedef uint64_t dbLong;  // 64 Bits (8 Bytes)
typedef float dbSingle;   // 32 Bits (4 bytes)
typedef double dbDouble;  // 64 Bits (8 Bytes)

struct dbIntDoublePair {
	dbInt first;
	dbDouble second;
};

struct dbTimingPoint {
	dbDouble beatTime;
	dbDouble offset;
	bool inherited;
};
typedef uint64_t dbDateTime; // 64 Bits (8 Bytes)

/*
Name	Number of bytes	Description
Int-Double pair	14	The first byte is 0x08, followed by an Int, then 0x0d, followed by a Double.
					These extraneous bytes are presumably flags to signify different data types in these slots,
					though in practice no other such flags have been seen. Currently the purpose of this data type is unknown.

Timing point	17	Consists of a Double, signifying the BPM, another Double, signifying the offset into the song,
					in milliseconds, and a Boolean; if false, then this timing point is inherited.
					See .osu (file format) for more information regarding timing points.

DateTime		8	A 64-bit number of ticks representing a date and time.
					Ticks are the amount of 100-nanosecond intervals since midnight, January 1, 0001 UTC.
*/

dbInt getULEB128(FILE*& f) {
	dbInt result   = 0;
	uint64_t shift = 0;
	while (true) {
		dbByte b = 0;
		fread(&b, 1, 1, f);

		result |= (b & 0x7f) << shift;
		if ((b & 0x80) == 0) // MSB is high, meaning its the last byte
			break;
		shift += 7;
	}
	return result;
}

#pragma region Version
dbInt dbVer;
dbInt getdbVer(FILE*& f) {
	dbInt _dbVer = 0;

	fread(&_dbVer, sizeof(_dbVer), 1, f);
	return _dbVer;
}
#pragma endregion

#pragma region FolderCount
dbInt getFolderCount(FILE*& f) {
	dbInt folderCount = 0;
	fread(&folderCount, sizeof(folderCount), 1, f);
	return folderCount;
}
#pragma endregion

#pragma region AccountUnlocked
bool getAccountUnlocked(FILE*& f) {
	bool accountUnlocked = 0;
	fread(&accountUnlocked, sizeof(accountUnlocked), 1, f);
	return accountUnlocked;
}
#pragma endregion

#pragma region UnlockDate
dbDateTime getUnlockDate(FILE*& f) {
	dbDateTime unlockDate = 0;
	fread(&unlockDate, sizeof(unlockDate), 1, f);
	return unlockDate;
}
#pragma endregion

#pragma region String
/*
Has three parts; a single byte which will be either 0x00, indicating that the
next two parts are not present, or 0x0b (decimal 11), indicating that the next
two parts are present. If it is 0x0b, there will then be a ULEB128, representing
the byte length of the following string, and then the string itself, encoded in UTF-8
*/

struct dbString {
	bool isPresent;
	dbByte rawIsPresent;

	size_t length;

	char* string;
};

dbString getString(FILE*& f) {
	dbByte first = 0;
	fread(&first, 1, 1, f);

	dbString str = {0};

	if (first == 0x0b) {
		str.isPresent    = true;
		str.rawIsPresent = 0x0b;

		dbInt length = getULEB128(f);
		str.length   = length;

		char* string = new char[length + 1];
		fread(string, length, 1, f);
		string[length] = 0;
		str.string     = string;
	} else {
		str.isPresent    = false;
		str.rawIsPresent = first;
	}

	return str;
};
#pragma endregion

#pragma region Beatmaps Count
dbInt getBeatmapsCount(FILE*& f) {
	dbInt beatmapsCount = 0;
	fread(&beatmapsCount, sizeof(beatmapsCount), 1, f);
	return beatmapsCount;
}
#pragma endregion

#pragma region Beatmaps

struct dbBeatmap {
	dbInt size; // Only present if dbVer < 20140609
	dbString artist;
	dbString artistUnicode;
	dbString title;
	dbString titleUnicode;
	dbString creator;
	dbString difficulty;
	dbString audioName;
	dbString md5;
	dbString osuName;
	dbByte status;
	dbShort hitcircles;
	dbShort sliders;
	dbShort spinners;
	dbDateTime lastModification;
	dbSingle ar;
	dbSingle cs;
	dbSingle hp;
	dbSingle od;
	dbDouble sliderVelocity;
	dbInt standardStarRatingCount;
	dbIntDoublePair* standardStarRating;
	dbInt taikoStarRatingCount;
	dbIntDoublePair* taikoStarRating;
	dbInt ctbStarRatingCount;
	dbIntDoublePair* ctbStarRating;
	dbInt maniaStarRatingCount;
	dbIntDoublePair* maniaStarRating;
	dbInt drainTime;
	dbInt totalTime;
	dbInt previewMilis;
	dbTimingPoint* timingPoints;
	dbInt difficultyId;
	dbInt beatmapId;
	dbInt threadId;
	dbByte standardRank;
	dbByte taikoRank;
	dbByte ctbRank;
	dbByte maniaRank;
	dbShort offset;
	dbSingle stackLeniency;
	dbByte mode;
	dbString source;
	dbString tags;
	dbShort onlineOffset;
	dbString titleFont;
	bool unplayed;
	dbLong lastTimePlayed;
	bool isOsz2;
	dbString folderName;
	dbLong lastCheck;
	bool ignoreSound;
	bool ignoreSkin;
	bool disableStoryboard;
	bool disableVideo;
	bool visualOverride;
	dbShort unknown;      // Only present if dbVer < 20140609
	dbInt lastModifyDate; // ?
	dbByte scrollSpeed;
};

dbBeatmap getBeatmap(FILE*& f) {
	dbBeatmap beatmap = {0};

	if (dbVer < 20191106)
		fread(&beatmap.size, sizeof(beatmap.size), 1, f);

	beatmap.artist        = getString(f);
	beatmap.artistUnicode = getString(f);

	beatmap.title        = getString(f);
	beatmap.titleUnicode = getString(f);

	beatmap.creator    = getString(f);
	beatmap.difficulty = getString(f);

	beatmap.audioName = getString(f);

	beatmap.md5     = getString(f);
	beatmap.osuName = getString(f);

	fread(&beatmap.status, sizeof(beatmap.status), 1, f);

	fread(&beatmap.hitcircles, sizeof(beatmap.hitcircles), 1, f);
	fread(&beatmap.sliders, sizeof(beatmap.sliders), 1, f);
	fread(&beatmap.spinners, sizeof(beatmap.spinners), 1, f);

	fread(&beatmap.lastModification, sizeof(beatmap.lastModification), 1, f);

	if (dbVer < 20140609) {
		dbByte ar = 0;
		fread(&ar, sizeof(ar), 1, f);
		beatmap.ar = (dbSingle)ar;

		dbByte cs = 0;
		fread(&cs, sizeof(cs), 1, f);
		beatmap.cs = (dbSingle)cs;

		dbByte hp = 0;
		fread(&hp, sizeof(hp), 1, f);
		beatmap.hp = (dbSingle)hp;

		dbByte od = 0;
		fread(&od, sizeof(od), 1, f);
		beatmap.od = (dbSingle)od;
	} else {
		dbSingle ar = 0;
		fread(&ar, sizeof(ar), 1, f);
		beatmap.ar = ar;

		dbSingle cs = 0;
		fread(&cs, sizeof(cs), 1, f);
		beatmap.cs = cs;

		dbSingle hp = 0;
		fread(&hp, sizeof(hp), 1, f);
		beatmap.hp = hp;

		dbSingle od = 0;
		fread(&beatmap.od, sizeof(od), 1, f);
	}

	fread(&beatmap.sliderVelocity, sizeof(beatmap.sliderVelocity), 1, f);

	dbByte flag = 0;
	// Standard Star Rating
	fread(&beatmap.standardStarRatingCount, sizeof(dbInt), 1, f);
	beatmap.standardStarRating = (dbIntDoublePair*)malloc(sizeof(dbIntDoublePair) * beatmap.standardStarRatingCount);
	for (dbInt i = 0; i < beatmap.standardStarRatingCount; i++) {
		fread(&flag, sizeof(dbByte), 1, f);
		assert(flag == 0x08);
		fread(&beatmap.standardStarRating[i].first, sizeof(beatmap.standardStarRating[i].first), 1, f);
		fread(&flag, sizeof(dbByte), 1, f);
		assert(flag == 0x0d);
		fread(&beatmap.standardStarRating[i].second, sizeof(beatmap.standardStarRating[i].second), 1, f);
	}

	// Taiko Star Rating
	fread(&beatmap.taikoStarRatingCount, sizeof(dbInt), 1, f);
	beatmap.taikoStarRating = (dbIntDoublePair*)malloc(sizeof(dbIntDoublePair) * beatmap.taikoStarRatingCount);
	for (dbInt i = 0; i < beatmap.taikoStarRatingCount; i++) {
		fread(&flag, sizeof(dbByte), 1, f);
		assert(flag == 0x08);
		fread(&beatmap.standardStarRating[i].first, sizeof(beatmap.standardStarRating[i].first), 1, f);
		fread(&flag, sizeof(dbByte), 1, f);
		assert(flag == 0x0d);
		fread(&beatmap.standardStarRating[i].second, sizeof(beatmap.standardStarRating[i].second), 1, f);
	}

	// CTB Star Rating
	fread(&beatmap.ctbStarRatingCount, sizeof(dbInt), 1, f);
	beatmap.ctbStarRating = (dbIntDoublePair*)malloc(sizeof(dbIntDoublePair) * beatmap.ctbStarRatingCount);
	for (dbInt i = 0; i < beatmap.ctbStarRatingCount; i++) {
		fread(&flag, sizeof(dbByte), 1, f);
		assert(flag == 0x08);
		fread(&beatmap.standardStarRating[i].first, sizeof(beatmap.standardStarRating[i].first), 1, f);
		fread(&flag, sizeof(dbByte), 1, f);
		assert(flag == 0x0d);
		fread(&beatmap.standardStarRating[i].second, sizeof(beatmap.standardStarRating[i].second), 1, f);
	}

	// Mania Star Rating
	fread(&beatmap.maniaStarRatingCount, sizeof(dbInt), 1, f);
	beatmap.maniaStarRating = (dbIntDoublePair*)malloc(sizeof(dbIntDoublePair) * beatmap.maniaStarRatingCount);
	for (dbInt i = 0; i < beatmap.maniaStarRatingCount; i++) {
		fread(&flag, sizeof(dbByte), 1, f);
		assert(flag == 0x08);
		fread(&beatmap.standardStarRating[i].first, sizeof(beatmap.standardStarRating[i].first), 1, f);
		fread(&flag, sizeof(dbByte), 1, f);
		assert(flag == 0x0d);
		fread(&beatmap.standardStarRating[i].second, sizeof(beatmap.standardStarRating[i].second), 1, f);
	}

	fread(&beatmap.drainTime, sizeof(beatmap.drainTime), 1, f);
	fread(&beatmap.totalTime, sizeof(beatmap.totalTime), 1, f);
	fread(&beatmap.previewMilis, sizeof(beatmap.previewMilis), 1, f);

	dbInt timingPointsCount = 0;
	fread(&timingPointsCount, sizeof(timingPointsCount), 1, f);

	beatmap.timingPoints = (dbTimingPoint*)malloc(sizeof(dbTimingPoint) * timingPointsCount);

	for (dbInt i = 0; i < timingPointsCount; i++) {
		dbDouble bpm;
		fread(&bpm, sizeof(bpm), 1, f);

		dbDouble offset;
		fread(&offset, sizeof(offset), 1, f);

		bool inherited;
		fread(&inherited, sizeof(inherited), 1, f);

		beatmap.timingPoints[i] = {bpm, offset, inherited};
	}

	fread(&beatmap.difficultyId, sizeof(beatmap.difficultyId), 1, f);
	fread(&beatmap.beatmapId, sizeof(beatmap.beatmapId), 1, f);
	fread(&beatmap.threadId, sizeof(beatmap.threadId), 1, f);
	fread(&beatmap.standardRank, sizeof(beatmap.standardRank), 1, f);
	fread(&beatmap.taikoRank, sizeof(beatmap.taikoRank), 1, f);
	fread(&beatmap.ctbRank, sizeof(beatmap.ctbRank), 1, f);
	fread(&beatmap.maniaRank, sizeof(beatmap.maniaRank), 1, f);
	fread(&beatmap.offset, sizeof(beatmap.offset), 1, f);
	fread(&beatmap.stackLeniency, sizeof(beatmap.stackLeniency), 1, f);
	fread(&beatmap.mode, sizeof(beatmap.mode), 1, f);

	beatmap.source = getString(f);
	beatmap.tags   = getString(f);

	fread(&beatmap.onlineOffset, sizeof(beatmap.onlineOffset), 1, f);

	beatmap.titleFont = getString(f);

	fread(&beatmap.unplayed, sizeof(beatmap.unplayed), 1, f);
	fread(&beatmap.lastTimePlayed, sizeof(beatmap.lastTimePlayed), 1, f);
	fread(&beatmap.isOsz2, sizeof(beatmap.isOsz2), 1, f);

	beatmap.folderName = getString(f);

	fread(&beatmap.lastCheck, sizeof(beatmap.lastCheck), 1, f);
	fread(&beatmap.ignoreSound, sizeof(beatmap.ignoreSound), 1, f);
	fread(&beatmap.ignoreSkin, sizeof(beatmap.ignoreSkin), 1, f);
	fread(&beatmap.disableStoryboard, sizeof(beatmap.disableStoryboard), 1, f);
	fread(&beatmap.disableVideo, sizeof(beatmap.disableVideo), 1, f);
	fread(&beatmap.visualOverride, sizeof(beatmap.visualOverride), 1, f);

	if (dbVer < 20140609)
		fread(&beatmap.unknown, sizeof(beatmap.unknown), 1, f);

	fread(&beatmap.lastModifyDate, sizeof(beatmap.lastModifyDate), 1, f);
	fread(&beatmap.scrollSpeed, sizeof(beatmap.scrollSpeed), 1, f);

	assert(beatmap.unknown == 0);
	return beatmap;
}

dbBeatmap* getBeatmaps(FILE*& f, dbInt beatmapCount) {
	dbBeatmap* beatmaps = (dbBeatmap*)malloc(sizeof(dbBeatmap) * beatmapCount);
	for (dbInt i = 0; i < beatmapCount; i++) {
		beatmaps[i] = getBeatmap(f);
	}
	return beatmaps;
}

#pragma endregion

#pragma region Permissions

#define PERMISSIONS_NORMAL        1 << 0
#define PERMISSIONS_MODERATOR     1 << 1
#define PERMISSIONS_SUPPORTER     1 << 2
#define PERMISSIONS_FRIEND        1 << 3
#define PERMISSIONS_PEPPY         1 << 4
#define PERMISSIONS_WORLDCUPSTAFF 1 << 5

struct permissions {
	int32_t raw;
	bool isNormal;
	bool isModerator;
	bool isSupporter;
	bool isFriend;
	bool isPeppy;
	bool isWorldCupStaff;
};

permissions getPermissions(FILE*& f) {
	int32_t _permissions = 0;
	fread(&_permissions, sizeof(int32_t), 1, f);

	permissions perms = {0};
	perms.raw         = _permissions;

	if (perms.raw & PERMISSIONS_NORMAL)
		perms.isNormal = true;
	if (perms.raw & PERMISSIONS_MODERATOR)
		perms.isModerator = true;
	if (perms.raw & PERMISSIONS_SUPPORTER)
		perms.isSupporter = true;
	if (perms.raw & PERMISSIONS_FRIEND)
		perms.isFriend = true;
	if (perms.raw & PERMISSIONS_PEPPY)
		perms.isPeppy = true;
	if (perms.raw & PERMISSIONS_WORLDCUPSTAFF)
		perms.isWorldCupStaff = true;

	return perms;
};

#pragma endregion

int main() {
	FILE* f = fopen("./main.db", "rb");

	dbVer                 = getdbVer(f);
	dbInt folderCount     = getFolderCount(f);
	bool accountUnlocked  = getAccountUnlocked(f);
	dbDateTime unlockDate = getUnlockDate(f);

	dbString playerName = getString(f);

	dbInt beatmapsCount = getBeatmapsCount(f);

	dbBeatmap* beatmaps = getBeatmaps(f, beatmapsCount);

	permissions perms = getPermissions(f);

	fclose(f);
}
