#pragma once
#include <vector>
#include <deque>
#include <string>

struct BBox 
{
	int left = 0, top = 0, right = 0, bottom = 0;
	int classId = 0;

	bool operator==(const BBox& o) const
	{
		return left == o.left && top == o.top && right == o.right && bottom == o.bottom && classId == o.classId;
	}
	bool operator!=(const BBox& o) const 
	{ 
		return !(*this == o); 
	}
};

struct DocumentSnapshot
{
	std::vector<BBox> bboxes;
	int selectedIndex = -1;
	std::wstring label;
};

class History
{
public:
	explicit History(size_t maxDepth = 200) : maxDepth_(maxDepth){}

	void Reset(const std::vector<BBox>& bboxes, int selectedIndex)
	{
		undoStack_.clear();
		redoStack_.clear();
		undoStack_.push_back({ bboxes, selectedIndex, L"Initial status" });
	}

	void Clear()
	{
		undoStack_.clear();
		redoStack_.clear();
	}

	void Commit(const std::vector<BBox>& bboxes, int selectedIndex, const std::wstring& label)
	{
		redoStack_.clear();
		undoStack_.push_back({ bboxes, selectedIndex, label });
		while (undoStack_.size() > maxDepth_)
		{
			undoStack_.pop_front();
		}
	}

	bool CanUndo() const { return undoStack_.size() > 1; }
	bool CanRedo() const { return !redoStack_.empty(); }

	bool Undo(std::vector<BBox>& outBoxes, int& outSel, std::wstring& outLabel) {
		if (!CanUndo()) return false;
		redoStack_.push_back(undoStack_.back());
		undoStack_.pop_back();
		const DocumentSnapshot& s = undoStack_.back();
		outBoxes = s.bboxes;
		outSel = s.selectedIndex;
		outLabel = s.label;
		return true;
	}

	bool Redo(std::vector<BBox>& outBoxes, int& outSel, std::wstring& outLabel) {
		if (!CanRedo()) return false;
		undoStack_.push_back(redoStack_.back());
		redoStack_.pop_back();
		const DocumentSnapshot& s = undoStack_.back();
		outBoxes = s.bboxes;
		outSel = s.selectedIndex;
		outLabel = s.label;
		return true;
	}

private:
	std::deque<DocumentSnapshot> undoStack_;
	std::deque<DocumentSnapshot> redoStack_;
	size_t maxDepth_;
};