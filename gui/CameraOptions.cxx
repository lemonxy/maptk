/*ckwg +29
 * Copyright 2016 by Kitware, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither the name Kitware, Inc. nor the names of any contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "CameraOptions.h"

#include "ui_CameraOptions.h"

#include "vtkMaptkCameraRepresentation.h"

#include <vtkActor.h>
#include <vtkProperty.h>

#include <qtMath.h>
#include <qtUiState.h>
#include <qtUiStateItem.h>

namespace // anonymous
{

//-----------------------------------------------------------------------------
double scaleValue(qtDoubleSlider* slider)
{
  return qPow(10.0, slider->value());
}

}

//-----------------------------------------------------------------------------
class CameraOptionsPrivate
{
public:
  CameraOptionsPrivate() : visibility(true), baseCameraScale(10.0) {}

  double activeScale() const;
  double inactiveScale() const;

  void mapUiState(QString const& key, QSlider* slider);
  void mapUiState(QString const& key, qtDoubleSlider* slider);

  Ui::CameraOptions UI;
  qtUiState uiState;

  vtkMaptkCameraRepresentation* representation;

  bool visibility;
  double baseCameraScale;
};

QTE_IMPLEMENT_D_FUNC(CameraOptions)

//-----------------------------------------------------------------------------
double CameraOptionsPrivate::activeScale() const
{
  return this->baseCameraScale * scaleValue(this->UI.scale);
}

//-----------------------------------------------------------------------------
double CameraOptionsPrivate::inactiveScale() const
{
  return this->activeScale() * scaleValue(this->UI.inactiveScale);
}

//-----------------------------------------------------------------------------
void CameraOptionsPrivate::mapUiState(
  QString const& key, QSlider* slider)
{
  auto const item = new qtUiState::Item<int, QSlider>(
    slider, &QSlider::value, &QSlider::setValue);
  this->uiState.map(key, item);
}

//-----------------------------------------------------------------------------
void CameraOptionsPrivate::mapUiState(
  QString const& key, qtDoubleSlider* slider)
{
  auto const item = new qtUiState::Item<double, qtDoubleSlider>(
    slider, &qtDoubleSlider::value, &qtDoubleSlider::setValue);
  this->uiState.map(key, item);
}

//-----------------------------------------------------------------------------
CameraOptions::CameraOptions(vtkMaptkCameraRepresentation* rep,
                             QWidget* parent, Qt::WindowFlags flags)
  : QWidget(parent, flags), d_ptr(new CameraOptionsPrivate)
{
  QTE_D();

  // Set up UI
  d->UI.setupUi(this);

  // Bundle inactive display modes into a group (mostly so we can have a single
  // signal when the mode changes, rather than toggled() for both the old and
  // new modes)
  auto const inactiveModeGroup = new QButtonGroup(this);
  inactiveModeGroup->addButton(d->UI.inactiveAsPoints);
  inactiveModeGroup->addButton(d->UI.inactiveAsFrustums);

  // Set up option persistence
  // TODO We may want to get a parent group from the user (of this class) that
  //      would identify which view in case of multiple views, so that we can
  //      have per-view persistence
  d->uiState.setCurrentGroup("Camera");

  d->UI.pathColor->persist(d->uiState, "Path/Color");
  d->UI.activeColor->persist(d->uiState, "Active/Color");
  d->UI.inactiveColor->persist(d->uiState, "Inactive/Color");

  d->mapUiState("Scale", d->UI.scale);
  d->mapUiState("Inactive/Scale", d->UI.inactiveScale);
  d->mapUiState("Inactive/PointSize", d->UI.inactivePointSize);

  d->uiState.mapChecked("Inactive", d->UI.showInactive);
  d->uiState.mapChecked("Inactive/Points", d->UI.inactiveAsPoints);
  d->uiState.mapChecked("Inactive/Frustums", d->UI.inactiveAsFrustums);
  d->uiState.mapChecked("Path", d->UI.showPath);

  d->uiState.restore();

  // Set up initial representation state
  d->representation = rep;

  d->UI.pathColor->addActor(rep->GetPathActor());
  d->UI.activeColor->addActor(rep->GetActiveActor());
  d->UI.inactiveColor->addActor(rep->GetNonActiveActor());

  updateScale();

  setPathVisible(d->UI.showPath->isChecked());
  setInactiveVisible(d->UI.showInactive->isChecked());

  // Connect signals/slots
  connect(d->UI.pathColor, SIGNAL(colorChanged(QColor)),
          this, SIGNAL(modified()));
  connect(d->UI.activeColor, SIGNAL(colorChanged(QColor)),
          this, SIGNAL(modified()));
  connect(d->UI.inactiveColor, SIGNAL(colorChanged(QColor)),
          this, SIGNAL(modified()));

  connect(d->UI.scale, SIGNAL(valueChanged(double)),
          this, SLOT(updateScale()));

  connect(inactiveModeGroup, SIGNAL(buttonClicked(QAbstractButton*)),
          this, SLOT(updateInactiveDisplayOptions()));
  connect(d->UI.inactivePointSize, SIGNAL(valueChanged(int)),
          this, SLOT(updateInactiveDisplayOptions()));
  connect(d->UI.inactiveScale, SIGNAL(valueChanged(double)),
          this, SLOT(updateInactiveDisplayOptions()));

  connect(d->UI.showPath, SIGNAL(toggled(bool)),
          this, SLOT(setPathVisible(bool)));
  connect(d->UI.showInactive, SIGNAL(toggled(bool)),
          this, SLOT(setInactiveVisible(bool)));
}

//-----------------------------------------------------------------------------
CameraOptions::~CameraOptions()
{
  QTE_D();
  d->uiState.save();
}

//-----------------------------------------------------------------------------
void CameraOptions::setBaseCameraScale(double s)
{
  QTE_D();

  if (s != d->baseCameraScale)
  {
    d->baseCameraScale = s;
    this->updateScale();
  }
}

//-----------------------------------------------------------------------------
void CameraOptions::setCamerasVisible(bool state)
{
  QTE_D();

  d->visibility = state;

  d->representation->GetActiveActor()->SetVisibility(state);
  d->representation->GetNonActiveActor()->SetVisibility(
    state && d->UI.showInactive->isChecked());
  d->representation->GetPathActor()->SetVisibility(
    state && d->UI.showPath->isChecked());

  emit this->modified();
}

//-----------------------------------------------------------------------------
void CameraOptions::setPathVisible(bool state)
{
  QTE_D();

  d->representation->GetPathActor()->SetVisibility(d->visibility && state);

  emit this->modified();
}

//-----------------------------------------------------------------------------
void CameraOptions::setInactiveVisible(bool state)
{
  QTE_D();

  d->representation->GetNonActiveActor()->SetVisibility(d->visibility && state);

  emit this->modified();
}

//-----------------------------------------------------------------------------
void CameraOptions::updateScale()
{
  QTE_D();

  d->representation->SetActiveCameraRepLength(d->activeScale());

  if (d->UI.inactiveAsFrustums->isChecked())
  {
    // Also update inactive scale; this will update the representation (which
    // we don't want to do twice, because it's expensive) and emit
    // this->modified()
    this->updateInactiveDisplayOptions();
  }
  else
  {
    d->representation->Update();
    emit this->modified();
  }
}

//-----------------------------------------------------------------------------
void CameraOptions::updateInactiveDisplayOptions()
{
  QTE_D();

  if (d->UI.inactiveAsFrustums->isChecked())
  {
    // TODO set display mode to frustums
    d->representation->SetNonActiveCameraRepLength(d->inactiveScale());
    d->representation->Update();
  }
  else
  {
    // TODO
  }

  emit this->modified();
}
