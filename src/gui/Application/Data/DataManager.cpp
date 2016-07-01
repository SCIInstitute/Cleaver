#include "DataManager.h"
#include <QMessageBox>
#include <Cleaver/InverseField.h>
#include <Cleaver/AbstractScalarField.h>

DataManager::DataManager() : mesh_(nullptr), sizingField_(nullptr), volume_(nullptr) {}


DataManager::~DataManager(){}

void DataManager::setMesh(cleaver::TetMesh *mesh) {
  this->mesh_ = mesh;
}

void DataManager::setSizingField(cleaver::AbstractScalarField *field) {
  this->sizingField_ = field;
  if (this->volume_) {
    this->volume_->setSizingField(field);
  }
}

void DataManager::setIndicators(std::vector<cleaver::AbstractScalarField *> indicators) {
  this->indicators_ = indicators;
}

void DataManager::setVolume(cleaver::Volume *volume) {
  this->volume_ = volume;
}

cleaver::AbstractScalarField*  DataManager::sizingField() const { return this->sizingField_; }

std::vector<cleaver::AbstractScalarField* > DataManager::indicators() const { return this->indicators_; }

cleaver::Volume*  DataManager::volume() const { return this->volume_; }

cleaver::TetMesh* DataManager::mesh() const { return this->mesh_; }