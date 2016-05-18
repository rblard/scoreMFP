#pragma once
#include <iscore/selection/Selection.hpp>
#include <iscore/tools/IdentifiedObject.hpp>
#include <QByteArray>
#include <QJsonObject>

#include <QString>
#include <QVariant>
#include <algorithm>
#include <iterator>
#include <vector>

class QObject;
#include <iscore/tools/SettableIdentifier.hpp>

namespace iscore
{
class DocumentDelegateFactory;
class DocumentDelegateModelInterface;
class DocumentPlugin;
struct ApplicationContext;

/**
     * @brief The DocumentModel class
     *
     * Drawbridge between the application and a model given by a plugin.
     * Contains all the "saveable" data.
     */
class ISCORE_LIB_BASE_EXPORT DocumentModel final : public IdentifiedObject<DocumentModel>
{
        Q_OBJECT
    public:
        DocumentModel(
                const Id<DocumentModel>& id,
                DocumentDelegateFactory& fact,
                QObject* parent);
        DocumentModel(
                iscore::DocumentContext& ctx,
                const QVariant &data,
                DocumentDelegateFactory& fact,
                QObject* parent);
        ~DocumentModel();

        DocumentDelegateModelInterface& modelDelegate() const
        {
            return *m_model;
        }

        // Plugin models
        void addPluginModel(DocumentPlugin* m);
        const std::vector<DocumentPlugin*>& pluginModels() { return m_pluginModels; }

    signals:
        void pluginModelsChanged();

    public slots:
        void setNewSelection(const Selection&);

    private:
        void loadDocumentAsJson(
                iscore::DocumentContext& ctx,
                const QJsonObject&,
                DocumentDelegateFactory& fact);
        void loadDocumentAsByteArray(
                iscore::DocumentContext& ctx,
                const QByteArray&,
                DocumentDelegateFactory& fact);

        std::vector<DocumentPlugin*> m_pluginModels;
        DocumentDelegateModelInterface* m_model{}; // note : this *has* to be last due to init order
};
}
