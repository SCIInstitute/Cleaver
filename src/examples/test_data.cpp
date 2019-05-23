#include <cleaver/Cleaver.h>
#include <cleaver/InverseField.h>
#include <cleaver/ConstantField.h>
#include <cleaver/vec3.h>
#include <cleaver/AbstractScalarField.h>
#include <cleaver/BoundingBox.h>

#include <Synthetic/PlaneField.h>
#include <Synthetic/PlaneSizingField.h>
#include <Synthetic/SphereField.h>
#include <Synthetic/SphereSizingField.h>
#include <Synthetic/TorusField.h>

#include <NRRDTools.h>

#include <string>
#include <vector>


void saveFields(const std::vector<cleaver::FloatField*> &fields);

int main(int argc, char *argv[]) {

  cleaver::BoundingBox bounds(
      cleaver::vec3::zero, cleaver::vec3(64,64,64));

  auto *f0 = new cleaver::ConstantField<float>(0, bounds);
  auto *f2 = new SphereField(cleaver::vec3(32,32,32), 20, bounds);
  auto *f1 = new TorusField(cleaver::vec3(32, 32, 32), 12, 5, bounds);

  std::vector<cleaver::AbstractScalarField*> fields;
  fields.push_back(f0);
  fields.push_back(f1);
  fields.push_back(f2);

  std::vector<cleaver::FloatField*> floatFields;
  for (int i=0; i < fields.size(); i++) {
    auto *f = cleaver::createFloatFieldFromScalarField(fields[i]);
    floatFields.push_back(f);
  }

  saveFields(floatFields);
  std::cout << "done." << std::endl;
}


void saveFields(const std::vector<cleaver::FloatField*> &fields) {

  for (int i=0; i < fields.size(); i++) {
    std::string filename = std::string("test_field") + std::to_string(i+1);
    std::cout << "Writing file: " << filename << ".nrrd" << std::endl;
    NRRDTools::saveNRRDFile(fields[i], filename);
  }
}
