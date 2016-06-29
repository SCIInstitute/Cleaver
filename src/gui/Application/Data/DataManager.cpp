#include "DataManager.h"
#include <QMessageBox>
#include <Cleaver/InverseField.h>
#include <Cleaver/AbstractScalarField.h>

DataManager::DataManager() : mesh_(NULL), sizingField_(NULL), volume_(NULL) {}


DataManager::~DataManager(){
  if (this->mesh_ != NULL) {
    delete this->mesh_;
  }
  if (this->sizingField_ != NULL) {
    delete this->sizingField_;
  }
  for (auto a : this->indicators_) {
    if (a != NULL) {
      delete a;
    }
  }
  if (this->volume_ != NULL) {
    delete this->volume_;
  }
}

void DataManager::setMesh(cleaver::TetMesh *mesh) {
  if (this->mesh_ != NULL) {
    delete this->mesh_;
  }
  this->mesh_ = mesh;
}

void DataManager::setSizingField(cleaver::AbstractScalarField *field) {
  if (this->sizingField_ != NULL) {
    delete this->sizingField_;
  }
  this->sizingField_ = field;
  this->volume_->setSizingField(field);
}

void DataManager::setIndicators(std::vector<cleaver::AbstractScalarField *> indicators) {
  for (auto a : this->indicators_) {
    if (a != NULL) {
      delete a;
    }
  }
  this->indicators_ = indicators;
}

void DataManager::setVolume(cleaver::Volume *volume) {
  if (this->volume_ != NULL) {
    delete this->volume_;
  }
  this->volume_ = volume;
}

cleaver::AbstractScalarField*  DataManager::sizingField() const { return this->sizingField_; }

std::vector<cleaver::AbstractScalarField* > DataManager::indicators() const { return this->indicators_; }

cleaver::Volume*  DataManager::volume() const { return this->volume_; }

cleaver::TetMesh* DataManager::mesh() const { return this->mesh_; }