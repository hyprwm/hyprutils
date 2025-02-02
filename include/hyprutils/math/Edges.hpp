#pragma once
#include <cstdint>

//NOLINTNEXTLINE
namespace Hyprutils::Math {

    /**
        * @brief Flag set of box edges
        */
    class CEdges {
      public:
        enum eEdges : uint8_t {
            NONE   = 0,
            TOP    = 1,
            LEFT   = 2,
            BOTTOM = 4,
            RIGHT  = 8,
        };

        CEdges() = default;
        CEdges(eEdges edges) : m_edges(edges) {}
        CEdges(uint8_t edges) : m_edges(static_cast<eEdges>(edges)) {}

        bool operator==(const CEdges& other) {
            return m_edges == other.m_edges;
        }

        CEdges operator|(const CEdges& other) {
            return m_edges | other.m_edges;
        }

        CEdges operator&(const CEdges& other) {
            return m_edges & other.m_edges;
        }

        CEdges operator^(const CEdges& other) {
            return m_edges ^ other.m_edges;
        }

        void operator|=(const CEdges& other) {
            m_edges = (*this | other).m_edges;
        }

        void operator&=(const CEdges& other) {
            m_edges = (*this & other).m_edges;
        }

        void operator^=(const CEdges& other) {
            m_edges = (*this ^ other).m_edges;
        }

        /**
            * @return if the edge set contains the top edge.
            */
        bool top() {
            return m_edges & TOP;
        }

        /**
            * @return if the edge set contains the left edge.
            */
        bool left() {
            return m_edges & LEFT;
        }

        /**
            * @return if the edge set contains the bottom edge.
            */
        bool bottom() {
            return m_edges & BOTTOM;
        }

        /**
            * @return if the edge set contains the right edge.
            */
        bool right() {
            return m_edges & RIGHT;
        }

        /**
            * @param top The state the top edge should be set to.
            */
        void setTop(bool top) {
            m_edges = static_cast<eEdges>((m_edges & ~TOP) | (TOP * top));
        }

        /**
            * @param left The state the left edge should be set to.
            */
        void setLeft(bool left) {
            m_edges = static_cast<eEdges>((m_edges & ~LEFT) | (LEFT * left));
        }

        /**
            * @param bottom The state the bottom edge should be set to.
            */
        void setBottom(bool bottom) {
            m_edges = static_cast<eEdges>((m_edges & ~BOTTOM) | (BOTTOM * bottom));
        }

        /**
            * @param right The state the right edge should be set to.
            */
        void setRight(bool right) {
            m_edges = static_cast<eEdges>((m_edges & ~RIGHT) | (RIGHT * right));
        }

        eEdges m_edges = NONE;
    };

}
