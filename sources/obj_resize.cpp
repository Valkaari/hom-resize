//
//  obj_resize.cpp
//  objectresize
//
//  Created by Manuel MAGALHAES on 22/08/2017.
//  Copyright © 2017 MAXON Computer GmbH. All rights reserved.
//

#include "c4d.h"
#include "c4d_symbols.h"
#include "obj_resize.hpp"

#define ID_OBJECT_RESIZE 1028518


Bool ObjectResizeDialog::CheckObjectType_(AtomArray *objList)
{
    if (!objList)
        return false;
    if (objList->GetCount()==0)
        return false;
    BaseObject* obj = nullptr;
    
    for (Int32 i = 0; i < objList->GetCount(); i++)
    {
        obj = (BaseObject*)objList->GetIndex(i);
        if (!obj->IsInstanceOf(Opolygon) && ((obj->GetInfo() & OBJECT_ISSPLINE) != OBJECT_ISSPLINE) )
           return false;
    }
    return true;
}

Bool ObjectResizeDialog::UpdateUI_()
{
    ActivateField_(false);
    
    BaseDocument* doc = GetActiveDocument();
    if (!doc)
        return false;
    AutoAlloc<AtomArray> selection;
    if (!selection)
        return false;
    doc->GetActiveObjects(*selection, GETACTIVEOBJECTFLAGS_0);
    if (selection->GetCount() ==0)
        return false;

    if (!CheckObjectType_(selection)) {
        SetUIValue_(0.0, 0.0, 0.0);
        return false;
    }
    
    
    Int32 docMode = doc->GetMode();
    
    
    if (docMode==Mpoints)
    {
        if (selection->GetCount() > 1 )
            SetUIValue_(-1.0, -1.0, -1.0, true);
        else
        {
            Vector actualSize (Vector(0));
            // get all points of all objects
            
            for (Int32 i = 0 ; i < selection->GetCount(); i++)
                actualSize = GetSelectionSize_(selection->GetIndex(i));
            
            ActivateField_(true);
            SetUIValue_(actualSize.x, actualSize.y, actualSize.z);
        }
        
    }
    
//    if (docMode==Medges)
//        GePrint("edges mode");
//    if (docMode==Mpolygons)
//        GePrint("poly mode");
//    if (docMode == Muvpolygons)
//        GePrint("uvpoly mode");
//    
    
    
    if ((docMode == Mmodel) || (docMode == Mobject))
    {
        // all selected object have good type
        ActivateField_(true);
        if (selection->GetCount() > 1 )
        {
            SetUIValue_(-1.0, -1.0, -1.0, true);
        }
        else
        {
            BaseObject *op = (BaseObject*)selection->GetIndex(0);
            if (!op)
                return false;
            Vector actualSize = GetObjectSize_(op);
            SetUIValue_(actualSize.x, actualSize.y, actualSize.z);
        }
        
    }
    
    return true;
}

Vector ObjectResizeDialog::GetObjectSize_(BaseObject *op)
{
    
    //TODO: the size is not correct if the tangent are going outside form of the spline
 
    
    
    if (!op)
        return Vector(0);
    return op->GetRad()*2 * op->GetAbsScale();
}
Bool ObjectResizeDialog::SetUIValue_(Float sizeX, Float sizeY, Float sizeZ, Bool tristate)
{
    
    if (!SetFloat(ID_VSIZEX, sizeX,-1.0e18,1.0e18,1.0,FORMAT_FLOAT,0.0,0.0, false,tristate))
        return false;
    if (!SetFloat(ID_VSIZEY, sizeY,-1.0e18,1.0e18,1.0,FORMAT_FLOAT,0.0,0.0, false,tristate))
        return false;
    if (!SetFloat(ID_VSIZEZ, sizeZ,-1.0e18,1.0e18,1.0,FORMAT_FLOAT,0.0,0.0, false,tristate))
        return false;
    return true;
}

Bool ObjectResizeDialog::ScaleObject_(BaseObject *op, Float &ratio)
{
    
    Vector* paddr = ToPoly(op)->GetPointW();
    if (!paddr)
        return false;
    if (CompareFloatTolerant(ratio, 0.0))
        return false;
    
    for (Int32 i = 0; i < ToPoly(op)->GetPointCount(); i++, paddr++)
    {
        *paddr *= ratio;
    }
    
    
    

    if ((op->GetInfo() & OBJECT_ISSPLINE) == OBJECT_ISSPLINE)
    {
        SplineObject* opSpline = (SplineObject*)op;
        if (opSpline->GetInterpolationType() == SPLINETYPE_BEZIER)
        {
            Tangent *top = opSpline->GetTangentW();
            for (Int32 i = 0 ; i < opSpline->GetTangentCount(); i++, top++)
            {
                top->vl *= ratio;
                top->vr *= ratio;
            }
        }
    }
    
    op->Message(MSG_UPDATE);
    
    return true;
}


Bool ObjectResizeDialog::ModifyScaleObject_()
{
    BaseDocument *doc = GetActiveDocument();
    
    AutoAlloc<AtomArray> selection;
    if (!selection)
        return false;
    doc->GetActiveObjects(*selection, GETACTIVEOBJECTFLAGS_0);
    if (selection->GetCount()==0)
        return false;
    if (!CheckObjectType_(selection))
        return false;
    
    
    if (selection->GetCount()>1)
    {
        Vector ratioUI = Vector(0.0);
        GetFloat(ID_VSIZEX, ratioUI.x);
        GetFloat(ID_VSIZEY, ratioUI.y);
        GetFloat(ID_VSIZEZ, ratioUI.z);
        doc->StartUndo();
        
        for (Int32 i = 0; i < selection->GetCount(); i++)
        {
            BaseObject* op = (BaseObject*)selection->GetIndex(i);
            Float ratio = 1.0;
            Vector currentSize = GetObjectSize_(op);
            if (!CompareFloatTolerant(ratioUI.x, 0.0))
            {
                ratio = ratioUI.x / currentSize.x;
                doc->AddUndo(UNDOTYPE_CHANGE, op);
                ScaleObject_(op, ratio);
            }

            else if (!CompareFloatTolerant(ratioUI.y, 0.0))
            {
                ratio = ratioUI.y / currentSize.y;
                doc->AddUndo(UNDOTYPE_CHANGE, op);
                ScaleObject_(op, ratio);
            }

            else if (!CompareFloatTolerant(ratioUI.z, 0.0))
            {
                ratio = ratioUI.z / currentSize.z;
                doc->AddUndo(UNDOTYPE_CHANGE, op);
                ScaleObject_(op, ratio);
            }

            
        }
        
        doc->EndUndo();
        
        
        
        
    }
    else
    {
        BaseObject *op = (BaseObject*)selection->GetIndex(0);
        Float ratio = GetRatio(GetObjectSize_(op));
        
        doc->StartUndo();
        doc->AddUndo(UNDOTYPE_CHANGE, op);
        ScaleObject_(op, ratio);
        
        doc->EndUndo();
        
        
        
    }
    
    
    
    return true;
}

Vector ObjectResizeDialog::GetSelectionSize_(C4DAtom* op)
{
    
    LMinMax bb;
    bb.Init();
    
    PointObject *obj = ToPoly(op);
    
    BaseSelect*  pbs = obj->GetPointS();
    const Vector *paddr  = obj->GetPointR();
    for (Int32 j = 0 ; j < obj->GetPointCount(); j++, paddr++)
    {
        if (pbs->IsSelected(j))
            bb.AddPoint(*paddr);
    }

    return bb.GetRad()*2;
}

Float ObjectResizeDialog::GetRatio(Vector actualSize)
{
    
    //avoid divide by zero
    Vector ratio = Vector(1.0);
    // avoid divide by 0
    if (CompareFloatTolerant(actualSize.x, 0.0))
        ratio.x = 1.0;
    else
    {
        GetFloat(ID_VSIZEX, ratio.x);
        ratio.x /= actualSize.x;
    }
    
    if (CompareFloatTolerant(actualSize.y, 0.0))
        ratio.y = 1.0;
    else
    {
        GetFloat(ID_VSIZEY, ratio.y);
        ratio.y /= actualSize.y;
    }
    if (CompareFloatTolerant(actualSize.z, 0.0))
        ratio.z = 1.0;
    else
    {
        GetFloat(ID_VSIZEY, ratio.z);
        ratio.z /= actualSize.z;
    }
    
    if (!CompareFloatTolerant(ratio.x, 1.0))
        return ratio.x;
    else if (!CompareFloatTolerant(ratio.y, 1.0))
        return ratio.y;
    else if (!CompareFloatTolerant(ratio.z, 1.0))
        return ratio.z;

    return 1.0;
}

Bool ObjectResizeDialog::ModifyScaleSelection_()
{
    BaseDocument* doc = GetActiveDocument();
    if (!doc)
        return false;
    AutoAlloc<AtomArray> selection;
    if (!selection)
        return false;
    doc->GetActiveObjects(*selection, GETACTIVEOBJECTFLAGS_0);
    if (selection->GetCount() ==0)
        return false;
    
    if (selection->GetCount()>1)
    {
        GePrint("multi selection");
    }
    else
    {
        PointObject *op  =(PointObject*)selection->GetIndex(0);
        
        const Matrix mdaxis = op->GetModelingAxis(doc);
        
        
        Vector *paddr = op->GetPointW();
        GetSelectionSize_(selection->GetIndex(0));
        BaseSelect *bs = op->GetPointS();
        const Matrix mg = op->GetMg();

        // calculate the ration
        
        
        
        Float ratio = GetRatio(GetObjectSize_(op));
        
        
        doc->StartUndo();
        doc->AddUndo(UNDOTYPE_CHANGE, op);
        for (Int32 i  = 0; i < op->GetPointCount(); i++, paddr++)
            if (bs->IsSelected(i))
                *paddr = ~mg * mdaxis * (ratio * (~mdaxis * mg * *paddr));
        
        if ((op->GetInfo() & OBJECT_ISSPLINE) == OBJECT_ISSPLINE)
        {
            SplineObject* opSpline = (SplineObject*)op;
            if (opSpline->GetInterpolationType() == SPLINETYPE_BEZIER)
            {
                Tangent *top = opSpline->GetTangentW();
                for (Int32 i = 0 ; i < opSpline->GetTangentCount(); i++, top++)
                {
                    if (bs->IsSelected(i))
                    {
                        top->vl *= ratio;
                        top->vr *= ratio;
                    }
                }
            }
        }
        
        doc->EndUndo();
        op->Message(MSG_UPDATE);
            
        
        
        
        
    }
    
    
    
    
    
    
    
    
    return true;
}


Bool ObjectResizeDialog::Modification_()
{
    BaseDocument* doc = GetActiveDocument();
    if (!doc)
        return false;
     Int32 docMode = doc->GetMode();
    switch (docMode) {
        case Mobject:
        case Mmodel:
            ModifyScaleObject_();
            break;
        case Mpoints:
            ModifyScaleSelection_();
            break;
    }
    
    
    EventAdd();
    return true;
}

void ObjectResizeDialog::ActivateField_(Bool status)
{
    Enable(ID_VSIZEX, status);
    Enable(ID_VSIZEY, status);
    Enable(ID_VSIZEZ, status);
}

Bool ObjectResizeDialog::InitValues()
{
    SetFloat(ID_VSIZEX, 0.0);
    SetFloat(ID_VSIZEY, 0.0);
    SetFloat(ID_VSIZEZ, 0.0);
    ActivateField_(false);
    UpdateUI_();
    return true;
}

Bool ObjectResizeDialog::CreateLayout()
{
    
    SetTitle(GeLoadString(DIALOG_TITLE));
    GroupBegin(SIZEGROUP, BFH_SCALEFIT, 3, 1, "", 0);
        GroupBegin(XGROUP, BFH_SCALEFIT, 2, 1, "", 0);
        AddStaticText(XSTR, BFH_FIT, 10, 10, "X", BORDER_NONE);
        AddEditNumber(ID_VSIZEX, BFH_SCALEFIT);
        GroupEnd();
    
        GroupBegin(YGROUP, BFH_SCALEFIT, 2, 1, "", 0);
        AddStaticText(YSTR, BFH_FIT, 10, 10, "Y", BORDER_NONE);
        AddEditNumber(ID_VSIZEY, BFH_SCALEFIT);
        GroupEnd();
    
        GroupBegin(ZGROUP, BFH_SCALEFIT, 2, 1, "", 0);
        AddStaticText(ZSTR, BFH_FIT, 10, 10, "Z", BORDER_NONE);
        AddEditNumber(ID_VSIZEZ, BFH_SCALEFIT);
        GroupEnd();
    
    GroupEnd();
    
    return true;
}

Bool ObjectResizeDialog::CoreMessage(Int32 id, const BaseContainer &msg)
{
    if (id == EVMSG_CHANGE)
        UpdateUI_();
    return true;
}

Bool ObjectResizeDialog::Command(Int32 id, const BaseContainer &msg)
{
    if (id == ID_VSIZEX || id==ID_VSIZEY || id==ID_VSIZEZ )
        Modification_();
    return true;
}


Bool ObjectResizeCommand::Execute(BaseDocument *doc)
{
    return dlg_.Open(DLG_TYPE_ASYNC, ID_OBJECT_RESIZE,-1,-1,400,50);
    
}
Bool ObjectResizeCommand::RestoreLayout(void *secret)
{
    return dlg_.RestoreLayout(DLG_TYPE_ASYNC, 0, secret);
    
}




Bool RegisterObjectResize(void);

Bool RegisterObjectResize() {
    Filename iconPath = GeGetPluginPath();
    iconPath += "res";  iconPath += "icons";  iconPath +="objectresize.tif";
    AutoAlloc<BaseBitmap> icon;
    icon->Init(iconPath.GetString());

    
      return RegisterCommandPlugin(ID_OBJECT_RESIZE, "Object Resize", 0, icon, "", ObjectResizeCommand::Alloc());
    
}
